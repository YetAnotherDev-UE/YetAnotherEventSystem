// Fill out your copyright notice in the Description page of Project Settings.

#include "LightSwitchActor.h"

ALightSwitchActor::ALightSwitchActor() {
	PrimaryActorTick.bCanEverTick = true;
}

void ALightSwitchActor::BeginPlay() {
	Super::BeginPlay();	
}

void ALightSwitchActor::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// Only call this once (Why not in 'BeginPlay'?: Make sure subscription logic ran before broadcasting)
	if (!bTriggeredOnce) {

		UE_LOG(LogTemp, Warning, TEXT("Broadcast Event!"));

		OnLightSwitchTurnedOn_One.Broadcast(); // Reacted to in Blueprint
		bTriggeredOnce = true;
	}
}

