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

template<typename... Args>
class TEvent {
public:
	TEvent() = default;
	~TEvent() { Clear(); }

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

	void Unsubscribe(const FEventHandle& a_handle) {
		if (!a_handle.IsValid()) return; // Check if the handle was actually distributed by subscribing to an event or if it was created by the user

		FScopeLock lock(&m_mutex);
		m_callbacks.Remove(a_handle.Id);
	}
#pragma endregion
	// Call listeners and prune
	void Broadcast(Args... args) {
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
			if(callback->IsValid()) callback->Invoke(args...); // Object might have been destroyed by prior callback -> check validity
		}
	}

	void Clear() {
		FScopeLock lock(&m_mutex);
		m_callbacks.Empty();
		m_nextID = 0;
	}

	int32 GetListenerCount() const { return m_callbacks.Num(); }

private:
	TMap<uint64, TSharedPtr<IEventCallback<Args...>>> m_callbacks;
	uint64 m_nextID{ 0 };

	// Thread safety
	mutable FCriticalSection m_mutex; // allow locking in const methods
};