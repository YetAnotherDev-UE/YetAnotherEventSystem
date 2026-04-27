// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventListenerComponent.h"

#include "GlobalEventSubsystem.h"
#include "GlobalEventHelper.h"

UGlobalEventListenerComponent::UGlobalEventListenerComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UGlobalEventListenerComponent::BeginPlay() {
	Super::BeginPlay();

	// Get the tags from the query
	TArray<FGameplayTag> TagsToListenFor = GlobalEventGate.EventConditionQuery.GetGameplayTagArray();

	// Subscribe to all selected tags
	for (const FGameplayTag& SelectedTag : TagsToListenFor) {
		FEventHandle Handle = UGlobalEventHelper::SubscribeToGlobalEvent(this, SelectedTag, this, [this, SelectedTag](UObject* Sender, const FInstancedStruct& Payload) {
			// Add the event to the received tags (checks uniqueness)
			ReceivedEvents.AddTag(SelectedTag);

			// Check if we pass the query conditions
			if (GlobalEventGate.EventConditionQuery.Matches(ReceivedEvents)) {
				OnPassedGlobalEventGate.Broadcast();
				if(bResetGateWhenPassed) ReceivedEvents.Reset();
			}

			OnReceivedGlobalEvent.Broadcast(SelectedTag, Sender, Payload);
		});

		ActiveHandles.Add(SelectedTag, Handle);
	}
}

void UGlobalEventListenerComponent::EndPlay(EEndPlayReason::Type EndPlayReason) {
	for (const auto& Pair : ActiveHandles) {
		UGlobalEventHelper::UnsubscribeFromGlobalEvent(this, Pair.Key, Pair.Value);
	}
	ActiveHandles.Empty();

	Super::EndPlay(EndPlayReason);
}

void UGlobalEventListenerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

