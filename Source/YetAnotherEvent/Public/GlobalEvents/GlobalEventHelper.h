// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GlobalEventSubsystem.h"
#include "GlobalEventPayload.h"
#include "Event/EventSystem.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GlobalEventHelper.generated.h"

UCLASS()
class YETANOTHEREVENT_API UGlobalEventHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Payload", WorldContext = "WorldContextObject"), Category = "Global Events")
	static void BroadcastGlobalEvent(const UObject* WorldContextObject, const FGlobalEventPayload& Payload, bool bPropagateToParents = true);

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
	static FEventHandle SubscribeToGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, UObject* ContextObject, TCallable&& InCallable) {
		if (!Tag.IsValid() || !IsValid(ContextObject)) {
			return FEventHandle{};
		}

		UGlobalEventSubsystem* EventSystem = GetEventSubsystem(WorldContextObject, TEXT("SubscribeToGlobalEvent"));
		if (!EventSystem) {
			return FEventHandle{};
		}

		return EventSystem->SubscribeToTag(Tag, ContextObject, Forward<TCallable>(InCallable));
	}

	template<typename TObject>
	static FEventHandle SubscribeToGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, TObject* ContextObject, void(TObject::* Method)(const FGlobalEventPayload&)) {
		if (!Tag.IsValid() || !IsValid(ContextObject) || !Method) {
			return FEventHandle{};
		}

		UGlobalEventSubsystem* EventSystem = GetEventSubsystem(WorldContextObject, TEXT("SubscribeToGlobalEvent"));
		if (!EventSystem) {
			return FEventHandle{};
		}

		return EventSystem->SubscribeToTag(Tag, ContextObject, Method);
	}

	static void UnsubscribeFromGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, FEventHandle Handle);

private:
	static UGlobalEventSubsystem* GetEventSubsystem(const UObject* WorldContextObject, const TCHAR* OperationName);
};
