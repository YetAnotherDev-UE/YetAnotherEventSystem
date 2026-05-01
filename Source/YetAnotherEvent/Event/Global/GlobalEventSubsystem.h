// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../EventSystem.h"
#include "GlobalEventPayload.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GlobalEventSubsystem.generated.h"

UCLASS()
class YETANOTHEREVENT_API UGlobalEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Automatically generate an empty struct whenever the payload pin is unconnected
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Payload"), Category = "Global Events")
	void BroadcastByTag(const FGlobalEventPayload& Payload, bool bPropagateToParents = true);

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
	FEventHandle SubscribeToTag(FGameplayTag Tag, UObject* ContextObject, TCallable&& InCallable) {
		// Adds a null pointer if the tag doesn't exist yet
		TSharedPtr<TEvent<const FGlobalEventPayload&>>& EventPtr = TaggedEvents.FindOrAdd(Tag);

		if (!EventPtr.IsValid()) { // Allocate a TEvent for the new tag
			EventPtr = MakeShared<TEvent<const FGlobalEventPayload&>>();
		}

		// Subscribe to the event
		return EventPtr->SubscribeLambda(ContextObject, Forward<TCallable>(InCallable));;
	}

	template<typename TObject>
	FEventHandle SubscribeToTag(FGameplayTag Tag, TObject* ContextObject, void(TObject::* Method)(UObject*, const FGlobalEventPayload&)) {
		TSharedPtr<TEvent<const FGlobalEventPayload&>>& EventPtr = TaggedEvents.FindOrAdd(Tag);

		if (!EventPtr.IsValid()) {
			EventPtr = MakeShared<TEvent<const FGlobalEventPayload&>>();
		}

		return EventPtr->SubscribeUObject(ContextObject, Method);
	}

	void Unsubscribe(FGameplayTag Tag, FEventHandle Handle);

private:
	// Maps a tag to a shared pointer holding the event
	TMap<FGameplayTag, TSharedPtr<TEvent<const FGlobalEventPayload&>>> TaggedEvents;
};
