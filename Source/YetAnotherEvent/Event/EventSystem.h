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

#if WITH_DEV_AUTOMATION_TESTS
	#define ENABLE_TEVENT_PROFILING 0
#else
	#define ENABLE_TEVENT_PROFILING 1
#endif

#if ENABLE_TEVENT_PROFILING
	#define TEVENT_PROFILER_SCOPE(Name) TRACE_CPUPROFILER_EVENT_SCOPE(Name)
#else
	#define TEVENT_PROFILER_SCOPE(Name)
#endif

#ifndef UEVENT
#define UEVENT(...) // Prevent compiler complains
#endif

// Type-safe struct
struct FEventHandle {
	uint64 Id = 0;
	bool IsValid() const { return Id != 0; } // Can never be 0 if used for an event

	bool operator==(const FEventHandle& Other) const {
		return Id == Other.Id;
	}

	friend uint32 GetTypeHash(const FEventHandle& Handle) {
		return GetTypeHash(Handle.Id);
	}
};

template<typename... Args>
class TEvent {
private:
	// Data-oriented flat struct
	struct FCallbackNode {
		uint64 ID;
		int32 Priority;
		bool bContext;
		bool bOneShot;

		TWeakObjectPtr<UObject> ContextObject;
		TFunction<void(Args...)> Callable;

		bool IsValid() const {
			if (bContext) return ContextObject.IsValid();
			return true;
		}
	};

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
		TWeakObjectPtr<TObject> WeakObj = Obj;

		// Capture the weak pointer and method in a lambda
		TFunction<void(Args...)> Func = [WeakObj, Method](Args... InArgs) {
			if (TObject* ValidObj = WeakObj.Get()) {
				(ValidObj->*Method)(InArgs...);
			}
		};

		FScopeLock Lock(&Mutex);

		const uint64 Id = ++NextID; // Can never be 0 (atomic increment)
		Callbacks.Emplace(Id, FCallbackNode{ Id, Priority, true, false, Obj, MoveTemp(Func) });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching (deferred rebuilding)

		return FEventHandle{ Id };
	}

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value) // avoid ambiguity with function pointers
		FEventHandle SubscribeLambda(TCallable&& InCallable, int32 Priority = 0) { // Subscription logic for lambdas
		TFunction<void(Args...)> Func(Forward<TCallable>(InCallable));

		FScopeLock Lock(&Mutex);
		const uint64 Id = ++NextID;
		Callbacks.Emplace(Id, FCallbackNode{ Id, Priority, false, false, nullptr, MoveTemp(Func) });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching

		return FEventHandle{ Id };
	}

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
		FEventHandle SubscribeLambda(UObject* ContextObject, TCallable&& InCallable, int32 Priority = 0) {
		check(ContextObject); // Ensure the context is valid at the time of subscription

		TFunction<void(Args...)> Func(Forward<TCallable>(InCallable));

		FScopeLock Lock(&Mutex);
		const uint64 Id = ++NextID;
		Callbacks.Emplace(Id, FCallbackNode{ Id, Priority, true, false, ContextObject, MoveTemp(Func) });
		bIsDirty.store(true, std::memory_order_release); // Flag dirty for sorting/caching

		return FEventHandle{ Id };
	}

	// Subscribes a lambda that will automatically unbind after its first execution
	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
		FEventHandle SubscribeOnce(UObject* ContextObject, TCallable&& InCallable, int32 Priority = 0) {
		check(ContextObject); // Ensure the context is valid at the time of subscription

		TSharedPtr<std::atomic<bool>> bHasFired = MakeShared<std::atomic<bool>>(false);
		TFunction<void(Args...)> Func = [bHasFired, Callable = Forward<TCallable>(InCallable)](Args... InArgs) mutable {
			bool bExpected = false;
			if (bHasFired->compare_exchange_strong(bExpected, true)) { // Guarantee only the first thread will swap this to true
				Callable(InArgs...);
			}
		};

		FScopeLock Lock(&Mutex);
		const uint64 Id = ++NextID;
		Callbacks.Emplace(Id, FCallbackNode{ Id, Priority, true, true, ContextObject, MoveTemp(Func) });
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
		TEVENT_PROFILER_SCOPE(TEvent_TotalBroadcastTime);

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

		TSharedPtr<TArray<FCallbackNode>> LocalCache{};
		{ // Only lock the thread to check if a callback is valid or to modify the collection (the actual callbacks can be outside the lock -> prevents deadlocks)
			FScopeLock Lock(&Mutex);

			// Only rebuild the cache and sort the array if someone newly subscribed/unsubscribed
			// NOTE: Don't do this instantly after subscribing, to prevent cases where we might subscribe in a loop and sort after each iteration.
			if (bIsDirty.exchange(false, std::memory_order_acq_rel)) {

				// Create a new array
				TSharedPtr<TArray<FCallbackNode>> NewCache = MakeShared<TArray<FCallbackNode>>();
				NewCache->Reserve(Callbacks.Num());

				// Use an iterator to safely remove dead listeners
				for (auto It = Callbacks.CreateIterator(); It; ++It) {
					if (It.Value().IsValid()) {
						NewCache->Add(It.Value());
					}
					else {
						It.RemoveCurrent();
					}
				}

				// Sort once
				NewCache->Sort([](const FCallbackNode& A, const FCallbackNode& B) {
					return A.Priority > B.Priority;
					});

				// Swap the pointer to the new, fully updated array
				ImmutableCache = NewCache;
			}

			// Copy the pointer (to ensure that if the ImmutableCache is replaced by another thread, the array we loop over still stays in RAM)
			LocalCache = ImmutableCache; // RCU snapshot
		}

		if (!LocalCache.IsValid() || LocalCache->IsEmpty()) return;

		// Store the IDs of one-Shots events that fired
		TArray<uint64, TInlineAllocator<16>> ExpiredOneShotIDs;

		// This now has zero pointer chasing, zero vtable lookups and zero cache misses
		for (const FCallbackNode& Node : *LocalCache) {
			if (Node.IsValid()) { // Might be invalid, even after pruning dead listeners, if an earlier callback destroyed the object for a later callback
				// Trace the execution time of each individual listener
				TEVENT_PROFILER_SCOPE(TEvent_ExecuteSingleListener);
				Node.Callable(InArgs...);

				if (Node.bOneShot) {
					ExpiredOneShotIDs.Add(Node.ID);
				}
			}
			else { // Listener died during this broadcast frame, flag for cleanup on the next broadcast
				bIsDirty.store(true, std::memory_order_release); // Technically outside of a lock, but is atomic
			}
		}

		// Clean one-shot events (do this in a batch, instead of locking every time we fire a one-shot event)
		// NOTE: Technically this could have a potential risk, where multiple threads call Broadcast() at the same time
		// before the one shot is being removed here -> this is why "bHasFired" was introduced to the one shot lambda
		if (ExpiredOneShotIDs.Num() > 0) {
			FScopeLock Lock(&Mutex);
			for (uint64 ExpiredID : ExpiredOneShotIDs) {
				Callbacks.Remove(ExpiredID);
			}
			bIsDirty.store(true, std::memory_order_release);
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

		// Do the same as in Broadcast()
		TSharedPtr<TArray<FCallbackNode>> LocalCache{};
		{
			FScopeLock Lock(&Mutex);
			if (bIsDirty.exchange(false, std::memory_order_acq_rel)) {
				TSharedPtr<TArray<FCallbackNode>> NewCache = MakeShared<TArray<FCallbackNode>>();
				NewCache->Reserve(Callbacks.Num());

				for (auto It = Callbacks.CreateIterator(); It; ++It) {
					if (It.Value().IsValid()) {
						NewCache->Add(It.Value());
					}
					else {
						It.RemoveCurrent();
					}
				}

				NewCache->Sort([](const FCallbackNode& A, const FCallbackNode& B) {
					return A.Priority > B.Priority;
					}); 

				ImmutableCache = NewCache;
			}
			LocalCache = ImmutableCache;
		}
		if (!LocalCache.IsValid() || LocalCache->IsEmpty()) return; // IsEmpty() is absolutely crucial here compared to in Broadcast(), to skip the FPSTicker logic

		MaxExecutionsPerFrame = FMath::Max(MaxExecutionsPerFrame, 1); // Never go below 1, else timer would run forever

		// Create a shared state block that survives across multiple frames
		struct FTaskState {
			TSharedPtr<TArray<FCallbackNode>> ImmutableData;
			int32 CurrentIndex = 0;
			TTuple<typename TDecay<Args>::Type...> CopiedArgs;
		};

		TSharedPtr<FTaskState> TaskState = MakeShared<FTaskState>();
		TaskState->ImmutableData = LocalCache; // Store a reference of the immutable cache, so it's kept alive across multiple frames
		TaskState->CopiedArgs = MakeTuple(InArgs...); // Deep copy

		// Create a shared pointer to hold the handles
		TSharedPtr<FTSTicker::FDelegateHandle> SafeHandle = MakeShared<FTSTicker::FDelegateHandle>();

		*SafeHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, TaskState, SafeHandle, MaxExecutionsPerFrame](float DeltaTime) -> bool {
			// Trace the total time of the slice
			TEVENT_PROFILER_SCOPE(TEvent_TotalSliceBroadcastTime);

			int32 ExecutedThisFrame = 0;
			const TArray<FCallbackNode>& DataArray = *(TaskState->ImmutableData);

			while (TaskState->CurrentIndex < DataArray.Num() && ExecutedThisFrame < MaxExecutionsPerFrame) {
				const FCallbackNode& TargetNode = DataArray[TaskState->CurrentIndex];

				// Verify the listener wasn't manually unsubscribed between frames
				bool bStillSubscribed = false;
				{
					FScopeLock Lock(&Mutex);
					bStillSubscribed = Callbacks.Contains(TargetNode.ID); // O(1)
				}

				if (bStillSubscribed && TargetNode.IsValid()) {
					TEVENT_PROFILER_SCOPE(TEvent_ExecuteSingleListener);
					TaskState->CopiedArgs.ApplyAfter([&TargetNode](auto&&... UnpackedArgs) {
						TargetNode.Callable(Forward<decltype(UnpackedArgs)>(UnpackedArgs)...);
					});
		
					if (TargetNode.bOneShot) {
						// Immediately delete it from the master dictionary -> won't be rebuild
						FScopeLock Lock(&Mutex);
						Callbacks.Remove(TargetNode.ID);
						bIsDirty.store(true, std::memory_order_release);
					}
				}
				else {
					// It died or unsubscribed, flag for cleanup
					bIsDirty.store(true, std::memory_order_release);
				}

				TaskState->CurrentIndex++;
				ExecutedThisFrame++;
			}

			bool RunAgain = TaskState->CurrentIndex < DataArray.Num();

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
	TMap<uint64, FCallbackNode> Callbacks{};

	// Never removed from/added to -> if the list of listeners changes, a new array is allocated and this pointer is swapped to it.
	TSharedPtr<TArray<FCallbackNode>> ImmutableCache{};

	// Rebuilding
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