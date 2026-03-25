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
	virtual void Invoke(Args... args) = 0;
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

	virtual void Invoke(Args... args) override {
		if (TObjectClass* pointerToUObject = WeakObj.Get()) {
			(pointerToUObject->*Method)(args...);
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

	virtual void Invoke(Args... args) override { Func(args...); }
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

	virtual void Invoke(Args... args) override {
		if (ContextObj.IsValid()) {
			Func(args...);
		}
	}

	virtual bool IsValid() const override {
		return ContextObj.IsValid();
	}
};

template<typename... Args>
class TEvent {
public:
	TEvent() = default;
	~TEvent() {
		FScopeLock lock(&m_mutex);

		// Cancel any pending async broadcasts before the event gets destroyed
		for (FTSTicker::FDelegateHandle& handle : m_asnycTickerHandles) {
			if (handle.IsValid()) {
				FTSTicker::GetCoreTicker().RemoveTicker(handle);
			}
		}

		// Clear array
		m_asnycTickerHandles.Empty();
		m_callbacks.Empty();
	}

#pragma region (Un)Subscription Logic
	template<typename TObject>
	FEventHandle SubscribeUObject(TObject* Obj, void(TObject::* Method)(Args...)) {
		check(Obj); // Fail Fast
		FScopeLock lock(&m_mutex);

		const uint64 id = ++m_nextID; // Can never be 0
		m_callbacks.Add(id, MakeShared<TUObjectCallback<TObject, Args...>>(Obj, Method));
		return FEventHandle{ id };
	}

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value) // avoid ambiguity with function pointers
		FEventHandle SubscribeLambda(TCallable&& InCallable) { // Subscription logic for lambdas
		TFunction<void(Args...)> Fn(Forward<TCallable>(InCallable));

		FScopeLock lock(&m_mutex);
		const uint64 id = ++m_nextID;
		m_callbacks.Add(id, MakeShared<TLambdaCallback<Args...>>(MoveTemp(Fn)));
		return FEventHandle{ id };
	}

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
		FEventHandle SubscribeLambda(UObject* ContextObject, TCallable&& InCallable) {
		check(ContextObject); // Ensure the context is valid at the time of subscription

		TFunction<void(Args...)> Fn(Forward<TCallable>(InCallable));

		FScopeLock lock(&m_mutex);
		const uint64 id = ++m_nextID;
		m_callbacks.Add(id, MakeShared<TContextLambdaCallback<Args...>>(ContextObject, MoveTemp(Fn)));
		return FEventHandle{ id };
	}

	void Unsubscribe(const FEventHandle& a_handle) {
		if (!a_handle.IsValid()) return; // Check if the handle was actually distributed by subscribing to an event or if it was created by the user

		FScopeLock lock(&m_mutex);
		m_callbacks.Remove(a_handle.Id);
	}
#pragma endregion
	// Call listeners and prune
	void Broadcast(Args... args) {
		// Trace the total time of the broadcast function
		TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_TotalBroadcastTime);

		// Check if should be routed through the network first
		if (NetworkInterceptor && !m_isExecutingInterceptor) {
			m_isExecutingInterceptor = true;

			// Returns true, if the interceptor successfully sent a network package.
			// Stop execution -> the RPC will travel over the network and call 'Broadcast()' again
			bool wasIntercepted = NetworkInterceptor(args...);

			// Reset so future local broadcasts can fire
			m_isExecutingInterceptor = false;

			if (wasIntercepted) return;
		}

		TArray<TSharedPtr<IEventCallback<Args...>>, TInlineAllocator<16>> callbacksToInvoke;

		{ // Only lock the thread to check if a callback is valid or to modify the collection (the actual callbacks can be outside the lock -> prevents deadlocks)
			FScopeLock lock(&m_mutex);
			TArray<uint64, TInlineAllocator<16>> callbacksToRemove;
			callbacksToRemove.Reserve(m_callbacks.Num() / 4 + 1); // Assume 25% of callbacks are dead

			for (auto& pair : m_callbacks) {
				if (!pair.Value->IsValid()) {
					callbacksToRemove.Add(pair.Key);
				}
				else {
					callbacksToInvoke.Add(pair.Value);
				}
			}

			// Remove the dead callbacks
			for (uint64 deadID : callbacksToRemove) {
				m_callbacks.Remove(deadID);
			}
		}

		for (const TSharedPtr<IEventCallback<Args...>>& callback : callbacksToInvoke) {
			if (callback->IsValid()) {
				// Trace the execution time of each individual listener
				TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_ExecuteSingleListener);

				callback->Invoke(args...); // Object might have been destroyed by prior callback -> check validity
			}
		}
	}

	void SlicedBroadcast(int32 MaxExecutionsPerFrame, Args... args) {
		// Check if this should be routed through the network
		if (SlicedNetworkInterceptor && !m_isExecutingInterceptor) {
			m_isExecutingInterceptor = true;
			bool wasIntercepted = SlicedNetworkInterceptor(MaxExecutionsPerFrame, args...);
			m_isExecutingInterceptor = false;
			if (wasIntercepted) return;
		}

		TArray<uint64> idsToInvoke; // Don't use 'TInlineAllocator' here, else 'MoveTemp' would perform a deep copy
		{
			FScopeLock lock(&m_mutex);
			idsToInvoke.Reserve(m_callbacks.Num());
			for (auto& pair : m_callbacks) {
				if (pair.Value->IsValid()) idsToInvoke.Add(pair.Key);
			}
		}

		if (idsToInvoke.IsEmpty()) return;

		MaxExecutionsPerFrame = FMath::Max(MaxExecutionsPerFrame, 1); // Never go below 1, else timer would run forever

		// Create a shared state block that survives across multiple frames
		struct FTaskState {
			TArray<uint64> IDs;
			int32 CurrentIndex = 0;
		};

		TSharedPtr<FTaskState> taskState = MakeShared<FTaskState>();
		taskState->IDs = MoveTemp(idsToInvoke);

		// Create a heap-allocated shared pointer to hold the handles
		TSharedPtr<FTSTicker::FDelegateHandle> safeHandle = MakeShared<FTSTicker::FDelegateHandle>();

		*safeHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, taskState, safeHandle, MaxExecutionsPerFrame, args...](float DeltaTime) -> bool {
			// Trace the total time of the slice
			TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_TotalSliceBroadcastTime);

			int32 executedThisFrame = 0;

			while (taskState->CurrentIndex < taskState->IDs.Num() && executedThisFrame < MaxExecutionsPerFrame) {
				uint64 targetID = taskState->IDs[taskState->CurrentIndex];

				// Check the callback map if the ID is still valid (not dead/unsubscribed)
				TSharedPtr<IEventCallback<Args...>> callback{ nullptr };
				{
					FScopeLock lock(&m_mutex);
					if (TSharedPtr<IEventCallback<Args...>>* foundCallback = m_callbacks.Find(targetID)) {
						callback = *foundCallback;
					}
				}

				if (callback && callback->IsValid()) {
					// Trace the execution time of each individual listener
					TRACE_CPUPROFILER_EVENT_SCOPE(TEvent_ExecuteSingleListener);
					callback->Invoke(args...);
				}

				taskState->CurrentIndex++;
				executedThisFrame++;
			}

			bool runAgain = taskState->CurrentIndex < taskState->IDs.Num();

			// Make sure to clean up the handle to avoid memory leaks
			if (!runAgain) {
				FScopeLock lock(&m_mutex);
				m_asnycTickerHandles.RemoveSingleSwap(*safeHandle);
			}

			// Returns true, if hasn't reached end -> run lambda again
			return runAgain;
			}));

		{
			FScopeLock lock(&m_mutex);

			// Store handle for later (in case event is destroyed before the ticker finishes)
			m_asnycTickerHandles.Add(*safeHandle);
		}
	}

	void Clear() {
		FScopeLock lock(&m_mutex);
		m_callbacks.Empty();
	}

	// A lambda for deciding whether the broadcast should be routed through the network
	TFunction<bool(Args...)> NetworkInterceptor;
	TFunction<bool(int32, Args...)> SlicedNetworkInterceptor;

	int32 GetListenerCount() const { return m_callbacks.Num(); }

private:
	TMap<uint64, TSharedPtr<IEventCallback<Args...>>> m_callbacks;
	uint64 m_nextID{ 0 };
	bool m_isExecutingInterceptor{ false };

	// Thread safety
	mutable FCriticalSection m_mutex; // allow locking in const methods

	// Store the handles -> needed e.g., if object with event calls 'AsyncBroadcast' but is then destroyed, the ticker will try to fire a dead event
	TArray<FTSTicker::FDelegateHandle> m_asnycTickerHandles;
};