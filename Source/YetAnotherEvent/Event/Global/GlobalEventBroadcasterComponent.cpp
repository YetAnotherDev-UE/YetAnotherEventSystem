// Copyright (c) 2026 Core Memory Entertainment GbR. All rights reserved.

#include "GlobalEventBroadcasterComponent.h"
#include "GlobalEventPayload.h"
#include "GlobalEventHelper.h"

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
	for (const FGameplayTag& EventTag : TagsToTrigger) {
		if (Channels.IsEmpty()) {
			FGlobalEventPayload Payload;
			Payload.EventTag = EventTag;
			Payload.Sender = this;
			Payload.Data = PayloadData;

			UGlobalEventHelper::BroadcastGlobalEvent(this, Payload);
		}
		else {
			for (const FGameplayTag& EventChannel : Channels) {
				FGlobalEventPayload Payload;
				Payload.Channel = EventChannel;
				Payload.EventTag = EventTag;
				Payload.Sender = this;
				Payload.Data = PayloadData;

				UGlobalEventHelper::BroadcastGlobalEvent(this, Payload);
			}
		}
	}
}

