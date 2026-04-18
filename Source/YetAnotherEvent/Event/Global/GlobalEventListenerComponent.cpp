// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventListenerComponent.h"

#include "GlobalEventSubsystem.h"

UGlobalEventListenerComponent::UGlobalEventListenerComponent() {
	PrimaryComponentTick.bCanEverTick = false;
}

void UGlobalEventListenerComponent::BeginPlay() {
	Super::BeginPlay();

	UGlobalEventSubsystem* EventSystem = GetWorld()->GetSubsystem<UGlobalEventSubsystem>();
	if (!EventSystem) return;

	// Subscribe to all selected tags
	for (const FGameplayTag& SelectedTag : TagsToListenFor) {
		EventSystem->SubscribeToTag(SelectedTag, this, [this, SelectedTag](UObject* Sender, const FInstancedStruct& Payload) {
			// Call our event
			OnGlobalEventFired.Broadcast(SelectedTag, Sender, Payload);
		});
	}
}

void UGlobalEventListenerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

