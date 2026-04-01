/*****************************************************************************
* Project: YetAnotherEvent
* File : EventSystem.h
* Author : Julian Serve
*
* CopyRight (c) 2026 YetAnotherDev. All Rights Reserved.
*
* Description:
* High-performance observer pattern specifically for Unreal Engine.
******************************************************************************/
#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include <atomic>

#ifndef UEVENT
#define UEVENT(...) // Prevent compiler complains
#endif

// Type-safe struct
struct FEventHandle
{
	uint64 Id = 0;
	bool IsValid() const { return Id != 0; } // Can never be 0 if used for an event
};

template<typename... Args>
class IEventCallback {
public:
	virtual ~IEventCallback() = default;
	virtual void Invoke(Args... InArgs) = 0;
	virtual bool IsValid() const = 0; // A callback is invalid if e.g., a UObject died
};

// An event wrapper for UObjects (auto-detects death)
template<typename TObjectClass, typename... Args>
class TUObjectCallback : public IEventCallback<Args...> {
public:
	using MethodType = void (TObjectClass::*)(Args...); // Expects a pointer to a member-method type

	TWeakObjectPtr<TObjectClass> WeakObj;
	MethodType Method;

	TUObjectCallback(TObjectClass* InObj, MethodType InMethod) : WeakObj(InObj), Method(InMethod) {}

	virtual void Invoke(Args... InArgs) override {
		if (TObjectClass* PointerToUObject = WeakObj.Get()) {
			(PointerToUObject->*Method)(InArgs...);
		}
	}

	virtual bool IsValid() const override {
		return WeakObj.IsValid();
	}
};

template<typename... Args>
class TLambdaCallback : public IEventCallback<Args...> {
public:
	TFunction<void(Args...)> Func;

	TLambdaCallback(TFunction<void(Args...)>&& InFunc) : Func(MoveTemp(InFunc)) {} // Prevent copy overhead

	virtual void Invoke(Args... InArgs) override { Func(InArgs...); }
	virtual bool IsValid() const override { return true; } // Needs to be managed by the user (Lambdas aren't tracked by the GC)
};

// An event wrapper for lambdas tied to a UObject's lifespan
template<typename... Args>
class TContextLambdaCallback : public IEventCallback<Args...> {
public:
	TWeakObjectPtr<UObject> ContextObj;
	TFunction<void(Args...)> Func;

	TContextLambdaCallback(UObject* InContext, TFunction<void(Args...)>&& InFunc)
		: ContextObj(InContext), Func(MoveTemp(InFunc)) {
	}

	virtual void Invoke(Args... InArgs) override {
		if (ContextObj.IsValid()) {
			Func(InArgs...);
		}
	}

	virtual bool IsValid() const override {
		return ContextObj.IsValid();
	}
};

// A lambda that fires once and then deletes itself
template<typename... Args>
class TOneShotContextLambdaCallback : public IEventCallback<Args...> {
public:
	TWeakObjectPtr<UObject> ContextObj;
	TFunction<void(Args...)> Func;

	TOneShotContextLambdaCallback(UObject* InContext, TFunction<void(Args...)>&& InFunc)
		: ContextObj(InContext), Func(MoveTemp(InFunc)) {
	}

	virtual void Invoke(Args... InArgs) override {
		if (!bHasFired && ContextObj.IsValid()) {
			bHasFired = true; // Lock, so the callback can never fire again
			Func(InArgs...);
		}
	}

	virtual void IsValid() const override {
		return !bHasFired && ContextObj.IsValid();
	}

private:
	bool bHasFired{};
};

template<typename... Args>
class TEvent {
public:
	TEvent() = default;
	~TEvent() {
		FScopeLock Lock(&Mutex);

		// Cancel any pending async broadcasts before the event gets destroyed
		for (FTSTicker::FDelegateHandle& Handle : AsnycTickerHandles) {
			if (Handle.IsValid()) {
				FTSTicker::GetCoreTicker().RemoveTicker(Handle);
			}
		}

		// Clear array
		AsnycTickerHandles.Empty();
		Callbacks.Empty();
	}

	void PruneDeadListeners() {
		FScopeLock lock(&Mutex);

		TArray<uint64, TInlineAllocator<32>> KeysToRemove;
		for (auto& Pair : Callbacks) {
			if (!Pair.Value.Callback->IsValid()) {
				KeysToRemove.Add(Pair.Key);
			}
		}

		for (uint64 Id : KeysToRemove) {
			Callbacks.Remove(Id);
		}

		// If we removed something, flag the array to sort itself next time it broadcasts
		if (KeysToRemove.Num() > 0) {
			bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching
		}
	}

#pragma region (Un)Subscription Logic
	template<typename TObject>
	FEventHandle SubscribeUObject(TObject* Obj, void(TObject::* Method)(Args...), int32 Priority = 0) {
		check(Obj); // Fail Fast
		FScopeLock Lock(&Mutex);

		const uint64 Id = ++NextID; // Can never be 0 (atomic increment)
		Callbacks.Emplace(Id, FCallbackNode{ Id, MakeShared<TUObjectCallback<TObject, Args...>>(Obj, Method), Priority });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching

		return FEventHandle{ Id };
	}

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value) // avoid ambiguity with function pointers
		FEventHandle SubscribeLambda(TCallable&& InCallable, int32 Priority = 0) { // Subscription logic for lambdas
		TFunction<void(Args...)> Fn(Forward<TCallable>(InCallable));

		FScopeLock Lock(&Mutex);
		const uint64 Id = ++NextID;
		Callbacks.Emplace(Id, FCallbackNode{ Id, MakeShared<TLambdaCallback<Args...>>(MoveTemp(Fn)), Priority });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching

		return FEventHandle{ Id };
	}

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
		FEventHandle SubscribeLambda(UObject* ContextObject, TCallable&& InCallable, int32 Priority = 0) {
		check(ContextObject); // Ensure the context is valid at the time of subscription

		TFunction<void(Args...)> Fn(Forward<TCallable>(InCallable));

		FScopeLock Lock(&Mutex);
		const uint64 Id = ++NextID;
		Callbacks.Emplace(Id, FCallbackNode{ Id, MakeShared<TContextLambdaCallback<Args...>>(ContextObject, MoveTemp(Fn)), Priority });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching

		return FEventHandle{ Id };
	}

	// Subscribes a lambda that will automatically unbind after its first execution
	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
		FEventHandle SubscribeOnce(UObject* ContextObject, TCallable&& InCallable, int32 Priority = 0) {
		check(ContextObject); // Ensure the context is valid at the time of subscription

		TFunction<void(Args...)> Fn(Forward<TCallable>(InCallable));

		FScopeLock Lock(&Mutex);
		const uint64 Id = ++NextID;
		Callbacks.Emplace(Id, FCallbackNode{ Id, MakeShared<TOneShotContextLambdaCallback<Args...>>(ContextObject, MoveTemp(Fn)), Priority });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching

		return FEventHandle{ Id };
	}

	void Unsubscribe(const FEventHandle& InHandle) {
		if (!InHandle.IsValid()) return; // Check if the handle was actually distributed by subscribing to an event or if it was created by the user

		FScopeLock Lock(&Mutex);

		if (Callbacks.Remove(InHandle.Id) > 0) { // Make sure something is actually removed before forcing a resort/cache update
			bIsDirty.store(true, std::memory_order_release); // Flag dirty so cache removes listener
		}
	}
#pragma endregion
	// Call listeners and prune
	void Broadcast(Args... InArgs) {
		// Trace the total time of the broadcast function
		TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_TotalBroadcastTime);

		// Check if should be routed through the network first
		bool ExpectedInterceptor = false;
		if (NetworkInterceptor && bIsExecutingInterceptor.compare_exchange_strong(ExpectedInterceptor, true)) {
			// Returns true, if the interceptor successfully sent a network package.
			bool WasIntercepted = NetworkInterceptor(InArgs...);

			// Reset so future broadcasts can fire
			bIsExecutingInterceptor = false;

			// Stop execution -> the RPC will travel over the network and call 'Broadcast()' again
			if (WasIntercepted) return;
		}

		TArray<FCallbackNode, TInlineAllocator<16>> CallbacksToInvoke;

		{ // Only lock the thread to check if a callback is valid or to modify the collection (the actual callbacks can be outside the lock -> prevents deadlocks)
			FScopeLock Lock(&Mutex);

			// Only rebuild the cache and sort the array if someone newly subscribed/unsubscribed
			// NOTE: Don't do this instantly after subscribing, to prevent cases where we might subscribe in a loop and sort after each iteration.
			if (bIsDirty.exchange(false, std::memory_order_acq_rel)) {
				SortedCache.Empty(Callbacks.Num());

				// Use an iterator to safely remove dead listeners
				for (auto It = Callbacks.CreateIterator(); It; ++It) {
					if (It.Value().Callback->IsValid()) {
						SortedCache.Add(It.Value());
					}
					else {
						It.RemoveCurrent();
					}
				}

				// Sort once
				SortedCache.Sort([](const FCallbackNode& A, const FCallbackNode& B) {
					return A.Priority > B.Priority;
					});
			}

			// Copy the sorted cache into our local execution array.
			CallbacksToInvoke.Append(SortedCache);
		}

		for (const FCallbackNode& Node : CallbacksToInvoke) {
			if (Node.Callback->IsValid()) { // Might be invalid, even after pruning dead listeners, if an earlier callback destroyed the object for a later callback
				// Trace the execution time of each individual listener
				TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_ExecuteSingleListener);
				Node.Callback->Invoke(InArgs...);
			}
			else { // Listener died during this broadcast frame, flag for cleanup on the next broadcast
				bIsDirty.store(true, std::memory_order_release); // Technically outside of a lock, but is atomic
			}
		}
	}

	void SlicedBroadcast(int32 MaxExecutionsPerFrame, Args... InArgs) {
		// Check if this should be routed through the network
		bool ExpectedInterceptor = false;
		if (SlicedNetworkInterceptor && bIsExecutingInterceptor.compare_exchange_strong(ExpectedInterceptor, true)) {
			bool WasIntercepted = SlicedNetworkInterceptor(MaxExecutionsPerFrame, InArgs...);
			bIsExecutingInterceptor.store(false, std::memory_order_release);
			if (WasIntercepted) return;
		}

		TArray<uint64> IdsToInvoke; // Don't use 'TInlineAllocator' here, else 'MoveTemp' would perform a deep copy
		{
			FScopeLock Lock(&Mutex);

			// Only rebuild the cache if changed
			if (bIsDirty.exchange(false, std::memory_order_acq_rel)) {
				SortedCache.Empty(Callbacks.Num());

				for (auto It = Callbacks.CreateIterator(); It; ++It) {
					if (It.Value().Callback->IsValid()) {
						SortedCache.Add(It.Value());
					}
					else {
						It.RemoveCurrent();
					}
				}

				SortedCache.Sort([](const FCallbackNode& A, const FCallbackNode& B) {
					return A.Priority > B.Priority;
					});
			}

			// Populate the final array with the sorted IDs
			IdsToInvoke.Reserve(Callbacks.Num());
			for (const FCallbackNode& node : SortedCache) {
				IdsToInvoke.Add(node.ID);
			}
		}

		if (IdsToInvoke.IsEmpty()) return;

		MaxExecutionsPerFrame = FMath::Max(MaxExecutionsPerFrame, 1); // Never go below 1, else timer would run forever

		// Create a shared state block that survives across multiple frames
		struct FTaskState {
			TArray<uint64> IDs;
			int32 CurrentIndex = 0;
		};

		TSharedPtr<FTaskState> TaskState = MakeShared<FTaskState>();
		TaskState->IDs = MoveTemp(IdsToInvoke);

		// Create a shared pointer to hold the handles
		TSharedPtr<FTSTicker::FDelegateHandle> SafeHandle = MakeShared<FTSTicker::FDelegateHandle>();

		*SafeHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, TaskState, SafeHandle, MaxExecutionsPerFrame, InArgs...](float DeltaTime) -> bool {
			// Trace the total time of the slice
			TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_TotalSliceBroadcastTime);

			int32 ExecutedThisFrame = 0;

			while (TaskState->CurrentIndex < TaskState->IDs.Num() && ExecutedThisFrame < MaxExecutionsPerFrame) {
				uint64 TargetID = TaskState->IDs[TaskState->CurrentIndex];

				// Check if the ID is still valid (not dead/unsubscribed)
				TSharedPtr<IEventCallback<Args...>> Callback{ nullptr };
				{
					FScopeLock Lock(&Mutex);
					if (FCallbackNode* foundNode = Callbacks.Find(TargetID)) {
						Callback = foundNode->Callback;
					}
				}

				if (Callback && Callback->IsValid()) {
					// Trace the execution time of each individual listener
					TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_ExecuteSingleListener);
					Callback->Invoke(InArgs...);
				}
				else if (Callback && !Callback->IsValid()) {
					bIsDirty.store(true, std::memory_order_release); //  Rebuilt on the next standard broadcast
				}

				TaskState->CurrentIndex++;
				ExecutedThisFrame++;
			}

			bool RunAgain = TaskState->CurrentIndex < TaskState->IDs.Num();

			// Make sure to clean up the handle to avoid memory leaks
			if (!RunAgain) {
				FScopeLock Lock(&Mutex);
				AsnycTickerHandles.RemoveSingleSwap(*SafeHandle);
			}

			// Returns true, if hasn't reached end -> run lambda again
			return RunAgain;
			}));

		{
			FScopeLock Lock(&Mutex);

			// Store handle for later (in case event is destroyed before the ticker finishes)
			AsnycTickerHandles.Add(*SafeHandle);
		}
	}

	void Clear() {
		FScopeLock Lock(&Mutex);
		Callbacks.Empty();
		bIsDirty.store(true, std::memory_order_release);
	}

	// A lambda for deciding whether the broadcast should be routed through the network
	TFunction<bool(Args...)> NetworkInterceptor;
	TFunction<bool(int32, Args...)> SlicedNetworkInterceptor;

	int32 GetListenerCount() const { return Callbacks.Num(); }

private:
	struct FCallbackNode {
		uint64 ID; // Needed for ID extraction during 'SlicedBroadcast' cache reads
		TSharedPtr<IEventCallback<Args...>> Callback;
		int32 Priority;
	};

	TMap<uint64, FCallbackNode> Callbacks;
	TArray<FCallbackNode> SortedCache; // Fast, pre-sorted array
	std::atomic<bool> bIsDirty{ false };

	// Identification
	std::atomic<uint64> NextID{ 0 };

	// Networking
	std::atomic<bool> bIsExecutingInterceptor{ false };

	// Thread safety
	mutable FCriticalSection Mutex; // allow locking in const methods

	// Store the handles -> needed e.g., if object with event calls 'AsyncBroadcast' but is then destroyed, we need a way to remove the dead event
	TArray<FTSTicker::FDelegateHandle> AsnycTickerHandles;
};