// Fill out your copyright notice in the Description page of Project Settings.


#include "GlobalEventSubsystem.h"

void UGlobalEventSubsystem::BroadcastByTag(FGameplayTag Tag, UObject* Sender, const FInstancedStruct& Payload) {
	// Returns a pointer to our shared pointer
	if (TSharedPtr<TEvent<UObject*, const FInstancedStruct&>>* FoundEventPointer = TaggedEvents.Find(Tag)) {
		if (FoundEventPointer->IsValid()) {
			(*FoundEventPointer)->Broadcast(Sender, Payload);
		}
	}
}