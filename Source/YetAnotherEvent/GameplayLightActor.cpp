// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayLightActor.h"
#include "Components/LightComponent.h"
#include "LightSwitchActor.h"

AGameplayLightActor::AGameplayLightActor() {
	PrimaryActorTick.bCanEverTick = false;
}

void AGameplayLightActor::BeginPlay() {
	Super::BeginPlay();	
	
	// Disable visibility at the start (will be enabled via a blueprint event)
	m_attachedLight = FindComponentByClass<ULightComponent>();
	if (m_attachedLight) {
		m_attachedLight->SetVisibility(false);
	}
}

void AGameplayLightActor::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AGameplayLightActor::EndPlay(EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
}

void AGameplayLightActor::TurnLightOn() { } // Handled via Blueprint right now

