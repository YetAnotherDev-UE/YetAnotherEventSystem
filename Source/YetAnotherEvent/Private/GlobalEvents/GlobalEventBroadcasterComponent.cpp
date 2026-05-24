// Copyright (c) 2026 Core Memory Entertainment GbR. All rights reserved.

#include "GlobalEvents/GlobalEventBroadcasterComponent.h"

#include "GlobalEvents/GlobalEventHelper.h"
#include "GlobalEvents/GlobalEventPayload.h"
#include "YetAnotherEventLog.h"

UGlobalEventBroadcasterComponent::UGlobalEventBroadcasterComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UGlobalEventBroadcasterComponent::BeginPlay() {
	Super::BeginPlay();
}

void UGlobalEventBroadcasterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UGlobalEventBroadcasterComponent::TriggerEvents() {
	if (EventsToBroadcast.IsEmpty()) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventBroadcasterComponent on '%s' has no events to broadcast."), *GetNameSafe(GetOwner()));
		return;
	}

	UObject* SenderObject = GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(this);

	for (const FGlobalEventBroadcastEntry& Entry : EventsToBroadcast) {
		BroadcastEntry(Entry, SenderObject);
	}
}

void UGlobalEventBroadcasterComponent::BroadcastEntry(const FGlobalEventBroadcastEntry& Entry, UObject* SenderObject) const {
	if (!Entry.EventTag.IsValid()) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventBroadcasterComponent on '%s' skipped an entry with an invalid event tag."), *GetNameSafe(GetOwner()));
		return;
	}

	auto BroadcastWithChannel = [this, &Entry, SenderObject](const FGameplayTag& Channel) {
		FGlobalEventPayload Payload;
		Payload.Channel = Channel;
		Payload.EventTag = Entry.EventTag;
		Payload.Sender = SenderObject;
		Payload.Data = Entry.PayloadData;

		UGlobalEventHelper::BroadcastGlobalEvent(this, Payload, Entry.bPropagateToParents);
		};

	if (Entry.Channels.IsEmpty()) {
		BroadcastWithChannel(FGameplayTag{});
		return;
	}

	for (const FGameplayTag& Channel : Entry.Channels) {
		if (!Channel.IsValid()) {
			UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventBroadcasterComponent on '%s' skipped invalid channel for event '%s'."), *GetNameSafe(GetOwner()), *Entry.EventTag.ToString());
			continue;
		}

		BroadcastWithChannel(Channel);
	}
}
