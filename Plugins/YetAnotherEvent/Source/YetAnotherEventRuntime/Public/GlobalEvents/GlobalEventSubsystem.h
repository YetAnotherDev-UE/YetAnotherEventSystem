// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Event/EventSystem.h"
#include "GlobalEventPayload.h"

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GlobalEventSubsystem.generated.h"

UCLASS()
class YETANOTHEREVENTRUNTIME_API UGlobalEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Automatically generate an empty struct whenever the payload pin is unconnected.
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Payload"), Category = "Global Events")
	void BroadcastByTag(const FGlobalEventPayload& Payload, bool bPropagateToParents = true);

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
	FEventHandle SubscribeToTag(FGameplayTag Tag, UObject* ContextObject, TCallable&& InCallable) {
		if (!Tag.IsValid() || !IsValid(ContextObject)) {
			return FEventHandle{};
		}

		return FindOrAddEvent(Tag).SubscribeLambda(ContextObject, Forward<TCallable>(InCallable));
	}

	template<typename TObject>
	FEventHandle SubscribeToTag(FGameplayTag Tag, TObject* ContextObject, void(TObject::* Method)(const FGlobalEventPayload&)) {
		if (!Tag.IsValid() || !IsValid(ContextObject) || !Method) {
			return FEventHandle{};
		}

		return FindOrAddEvent(Tag).SubscribeUObject(ContextObject, Method);
	}

	void Unsubscribe(FGameplayTag Tag, FEventHandle Handle);

private:
	TEvent<const FGlobalEventPayload&>& FindOrAddEvent(FGameplayTag Tag);

	// Maps an event tag to its local listener event. Channels are currently filter metadata, not routing keys.
	TMap<FGameplayTag, TSharedPtr<TEvent<const FGlobalEventPayload&>>> TaggedEvents;
};
