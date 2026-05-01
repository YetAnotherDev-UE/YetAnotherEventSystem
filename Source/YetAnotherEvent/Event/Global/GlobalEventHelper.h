// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GlobalEventSubsystem.h"
#include "GlobalEventPayload.h"
#include "../EventSystem.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GlobalEventHelper.generated.h"

UCLASS()
class YETANOTHEREVENT_API UGlobalEventHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Payload", WorldContext = "WorldContextObject"), Category = "Own|Global Events")
	static void BroadcastGlobalEvent(const UObject* WorldContextObject, const FGlobalEventPayload& Payload, bool bPropagateToParents = true);

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
	static FEventHandle SubscribeToGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, UObject* ContextObject, TCallable&& InCallable) {
		if (!WorldContextObject) return FEventHandle{}; // Return an invalid handle if it fails
		UWorld* World = WorldContextObject->GetWorld();
		if (!World) return FEventHandle{};
		UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance) return FEventHandle{};
		UGlobalEventSubsystem* EventSystem = GameInstance->GetSubsystem<UGlobalEventSubsystem>();
		if (!EventSystem) return FEventHandle{};

		return EventSystem->SubscribeToTag(Tag, ContextObject, Forward<TCallable>(InCallable));
	}

	template<typename TObject>
	static FEventHandle SubscribeToGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, TObject* ContextObject, void(TObject::* Method)(UObject*, const FGlobalEventPayload&)) {
		if (!WorldContextObject) return FEventHandle{}; // Return an invalid handle if it fails
		UWorld* World = WorldContextObject->GetWorld();
		if (!World) return FEventHandle{};
		UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance) return FEventHandle{};
		UGlobalEventSubsystem* EventSystem = GameInstance->GetSubsystem<UGlobalEventSubsystem>();
		if (!EventSystem) return FEventHandle{};

		return EventSystem->SubscribeToTag(Tag, ContextObject, Method);
	}

	static void UnsubscribeFromGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, FEventHandle Handle);
};
