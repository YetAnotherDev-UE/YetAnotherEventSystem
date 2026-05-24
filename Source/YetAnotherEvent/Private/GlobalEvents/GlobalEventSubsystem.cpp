// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEvents/GlobalEventSubsystem.h"
#include "YetAnotherEventLog.h"

TEvent<const FGlobalEventPayload&>& UGlobalEventSubsystem::FindOrAddEvent(FGameplayTag Tag) {
	TSharedPtr<TEvent<const FGlobalEventPayload&>>& EventPtr = TaggedEvents.FindOrAdd(Tag);

	if (!EventPtr.IsValid()) {
		EventPtr = MakeShared<TEvent<const FGlobalEventPayload&>>();
	}

	return *EventPtr;
}

void UGlobalEventSubsystem::BroadcastByTag(const FGlobalEventPayload& Payload, bool bPropagateToParents) {
	if (!Payload.EventTag.IsValid()) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("BroadcastByTag failed: payload has no valid EventTag."));
		return;
	}

	if (bPropagateToParents) {
		const FGameplayTagContainer Hierarchy = Payload.EventTag.GetGameplayTagParents();

		// Propagate up the chain. GetGameplayTagParents includes the tag itself.
		for (const FGameplayTag& CurrentTag : Hierarchy) {
			if (const TSharedPtr<TEvent<const FGlobalEventPayload&>>* FoundEventPointer = TaggedEvents.Find(CurrentTag)) {
				if (FoundEventPointer->IsValid()) {
					(*FoundEventPointer)->Broadcast(Payload);
				}
			}
		}

		return;
	}

	if (const TSharedPtr<TEvent<const FGlobalEventPayload&>>* FoundEventPointer = TaggedEvents.Find(Payload.EventTag)) {
		if (FoundEventPointer->IsValid()) {
			(*FoundEventPointer)->Broadcast(Payload);
		}
	}
}

void UGlobalEventSubsystem::Unsubscribe(FGameplayTag Tag, FEventHandle Handle) {
	if (!Tag.IsValid() || !Handle.IsValid()) {
		return;
	}

	if (TSharedPtr<TEvent<const FGlobalEventPayload&>>* FoundEventPointer = TaggedEvents.Find(Tag)) {
		if (FoundEventPointer->IsValid()) {
			(*FoundEventPointer)->Unsubscribe(Handle);

			if ((*FoundEventPointer)->GetListenerCount() <= 0) {
				TaggedEvents.Remove(Tag);
			}
		}
	}
}
