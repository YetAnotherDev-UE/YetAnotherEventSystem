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
		// Will also pass channel (just needs to be updated)
		FEventHandle Handle = UGlobalEventHelper::SubscribeToGlobalEvent(this, SelectedTag, this, [this](const FGlobalEventPayload& Payload) {

			// Has specified channels but channel doesn't match
			if(!Channels.IsEmpty() && !Channels.HasTag(Payload.Channel)) return;
			
			// Update accumulator
			TSet<TWeakObjectPtr<UObject>>& UniqueSenders = SenderAccumulator.FindOrAdd(Payload.EventTag);
			UniqueSenders.Add(Payload.Sender);

			// Threshold check
			FThresholdValue* CustomThreshold = TagThresholds.Find(Payload.EventTag);
			int32 Requirement = CustomThreshold ? CustomThreshold->Value : 1;

			UE_LOG(LogTemp, Warning, TEXT("Actor: %s | Tag: %s | Senders: %d/%d"),
				*GetOwner()->GetName(),
				*Payload.EventTag.ToString(),
				UniqueSenders.Num(),
				Requirement);

			if (UniqueSenders.Num() >= Requirement) {
				// Add the event to the received tags (checks uniqueness)
				ReceivedEvents.AddTag(Payload.EventTag);
			}

			// Check if we pass the gate
			if (GlobalEventGate.EventConditionQuery.Matches(ReceivedEvents)) {
				OnPassedGlobalEventGate.Broadcast();

				if (bResetGateWhenPassed) {
					ReceivedEvents.Reset();
					SenderAccumulator.Empty();
				}
			}

			OnReceivedGlobalEvent.Broadcast(Payload);
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

