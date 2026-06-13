// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEvents/GlobalEventListenerComponent.h"

#include "GlobalEvents/GlobalEventHelper.h"
#include "YetAnotherEventLog.h"
#include "GameFramework/Actor.h"

UGlobalEventListenerComponent::UGlobalEventListenerComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UGlobalEventListenerComponent::BeginPlay() {
	Super::BeginPlay();

	if (EventsToListenFor.IsEmpty()) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventListenerComponent on '%s' has no event tags to listen for."), *GetNameSafe(GetOwner()));
		return;
	}

	if (GlobalEventGate.bEnabled && !GlobalEventGate.HasCondition()) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventListenerComponent on '%s' has gate enabled, but no gate condition query."), *GetNameSafe(GetOwner()));
	}

	for (const FGameplayTag& SelectedTag : EventsToListenFor) {
		if (!SelectedTag.IsValid()) {
			UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventListenerComponent on '%s' skipped an invalid event tag."), *GetNameSafe(GetOwner()));
			continue;
		}

		FEventHandle Handle = UGlobalEventHelper::SubscribeToGlobalEvent(this, SelectedTag, this, [this, SelectedTag](const FGlobalEventPayload& Payload) {
			HandleGlobalEvent(Payload, SelectedTag);
			});

		if (Handle.IsValid()) {
			ActiveHandles.Add(SelectedTag, Handle);
		}
		else {
			UE_LOG(LogYetAnotherEvent, Warning, TEXT("GlobalEventListenerComponent on '%s' failed to subscribe to tag '%s'."), *GetNameSafe(GetOwner()), *SelectedTag.ToString());
		}
	}
}

void UGlobalEventListenerComponent::EndPlay(EEndPlayReason::Type EndPlayReason) {
	for (const auto& Pair : ActiveHandles) {
		UGlobalEventHelper::UnsubscribeFromGlobalEvent(this, Pair.Key, Pair.Value);
	}

	ActiveHandles.Empty();
	ResetGateState();

	Super::EndPlay(EndPlayReason);
}

void UGlobalEventListenerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UGlobalEventListenerComponent::UsesGate() const {
	return GlobalEventGate.ShouldEvaluate();
}

void UGlobalEventListenerComponent::HandleGlobalEvent(const FGlobalEventPayload& Payload, FGameplayTag MatchedSubscriptionTag) {
	if (!PassesChannelFilter(Payload)) {
		UE_LOG(LogYetAnotherEvent, VeryVerbose, TEXT("GlobalEventListenerComponent on '%s' ignored event '%s' because channel '%s' did not match the configured channel filter."),
			*GetNameSafe(GetOwner()),
			*Payload.EventTag.ToString(),
			*Payload.Channel.ToString());
		return;
	}

	OnReceivedGlobalEvent.Broadcast(Payload);

	if (!GlobalEventGate.ShouldEvaluate()) {
		return;
	}

	AccumulateGateEvent(Payload, MatchedSubscriptionTag);

	if (GlobalEventGate.EventConditionQuery.Matches(ReceivedEvents)) {
		OnPassedGlobalEventGate.Broadcast();

		if (bResetGateWhenPassed) {
			ResetGateState();
		}
	}
}

bool UGlobalEventListenerComponent::PassesChannelFilter(const FGlobalEventPayload& Payload) const {
	return Channels.IsEmpty() || Channels.HasTag(Payload.Channel);
}

void UGlobalEventListenerComponent::AccumulateGateEvent(const FGlobalEventPayload& Payload, FGameplayTag MatchedSubscriptionTag) {
	const FGameplayTag GateTag = MatchedSubscriptionTag.IsValid() ? MatchedSubscriptionTag : Payload.EventTag;
	if (!GateTag.IsValid()) {
		return;
	}

	TSet<TWeakObjectPtr<UObject>>& UniqueSenders = SenderAccumulator.FindOrAdd(GateTag);
	UObject* SenderObject = Payload.Sender.Get();
	UniqueSenders.Add(SenderObject ? SenderObject : static_cast<UObject*>(this));

	const FThresholdValue* CustomThreshold = TagThresholds.Find(GateTag);
	const int32 Requirement = CustomThreshold ? CustomThreshold->Value : 1;

	UE_LOG(LogYetAnotherEvent, Verbose, TEXT("GlobalEventListenerComponent on '%s' received gate tag '%s': %d/%d unique sender(s)."),
		*GetNameSafe(GetOwner()),
		*GateTag.ToString(),
		UniqueSenders.Num(),
		Requirement);

	if (UniqueSenders.Num() >= Requirement) {
		ReceivedEvents.AddTag(GateTag);
	}
}

void UGlobalEventListenerComponent::ResetGateState() {
	ReceivedEvents.Reset();
	SenderAccumulator.Empty();
}
