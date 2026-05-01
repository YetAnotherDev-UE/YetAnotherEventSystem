// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventSubsystem.h"

void UGlobalEventSubsystem::BroadcastByTag(const FGlobalEventPayload& Payload, bool bPropagateToParents) {
	
	if (bPropagateToParents) {
		FGameplayTagContainer Hierarchy = Payload.EventTag.GetGameplayTagParents();

		// Propagate up the chain
		for (const FGameplayTag& CurrentTag : Hierarchy) {
			// Returns a pointer to our shared pointer
			if (auto* FoundEventPointer = TaggedEvents.Find(CurrentTag)) {
				if (FoundEventPointer->IsValid()) {
					(*FoundEventPointer)->Broadcast(Payload);
				}
			}
		}
	}
	// Strict match only
	else if (auto* FoundEventPointer = TaggedEvents.Find(Payload.EventTag)) {
		if (FoundEventPointer->IsValid()) {
			(*FoundEventPointer)->Broadcast(Payload);
		}
	}
}

void UGlobalEventSubsystem::Unsubscribe(FGameplayTag Tag, FEventHandle Handle) {
	if (auto* FoundEventPointer = TaggedEvents.Find(Tag)) {
		if (FoundEventPointer->IsValid()) {
			(*FoundEventPointer)->Unsubscribe(Handle);
		}
	}
}
