// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../EventSystem.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GlobalEventSubsystem.generated.h"

UCLASS()
class YETANOTHEREVENT_API UGlobalEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Automatically generate an empty struct whenever the payload pin is unconnected
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Payload"), Category = "Global Events")
	void BroadcastByTag(FGameplayTag Tag, UObject* Sender, const FInstancedStruct& Payload = FInstancedStruct());

	template<typename TCallable> requires (!TIsPointer<TCallable>::Value)
	FEventHandle SubscribeToTag(FGameplayTag Tag, UObject* ContextObject, TCallable&& InCallable) {
		// Adds a null pointer if the tag doesn't exist yet
		TSharedPtr<TEvent<UObject*, const FInstancedStruct&>>& EventPtr = TaggedEvents.FindOrAdd(Tag);

		if (!EventPtr.IsValid()) { // Allocate a TEvent for the new tag
			EventPtr = MakeShared<TEvent<UObject*, const FInstancedStruct&>>();
		}

		// Subscribe to the event
		return EventPtr->SubscribeLambda(ContextObject, Forward<TCallable>(InCallable));;
	}

	template<typename TObject>
	FEventHandle SubscribeToTag(FGameplayTag Tag, TObject* ContextObject, void(TObject::* Method)(UObject*, const FInstancedStruct&)) {
		TSharedPtr<TEvent<UObject*, const FInstancedStruct&>>& EventPtr = TaggedEvents.FindOrAdd(Tag);

		if (!EventPtr.IsValid()) {
			EventPtr = MakeShared<TEvent<UObject*, const FInstancedStruct&>>();
		}

		return EventPtr->SubscribeUObject(ContextObject, Method);
	}

private:
	// Maps a tag to a shared pointer holding the event
	// UObject: Sender, FInstancedStruct: Payload
	TMap<FGameplayTag, TSharedPtr<TEvent<UObject*, const FInstancedStruct&>>> TaggedEvents;
};
