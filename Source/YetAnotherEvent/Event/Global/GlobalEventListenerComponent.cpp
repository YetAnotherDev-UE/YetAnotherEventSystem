// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventListenerComponent.h"

#include "GlobalEventSubsystem.h"
#include "GlobalEventHelper.h"

UGlobalEventListenerComponent::UGlobalEventListenerComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UGlobalEventListenerComponent::BeginPlay() {
	Super::BeginPlay();

	// Subscribe to all selected tags
	for (const FGameplayTag& SelectedTag : TagsToListenFor) {
		
		FEventHandle Handle = UGlobalEventHelper::SubscribeToGlobalEvent(this, SelectedTag, this, [this, SelectedTag](UObject* Sender, const FInstancedStruct& Payload) {
			OnGlobalEventFired.Broadcast(SelectedTag, Sender, Payload);
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

