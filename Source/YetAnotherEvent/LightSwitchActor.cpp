// Fill out your copyright notice in the Description page of Project Settings.

#include "LightSwitchActor.h"

ALightSwitchActor::ALightSwitchActor() {
	PrimaryActorTick.bCanEverTick = true;
}

void ALightSwitchActor::Tick(float DeltaTime) {
	// Only call this once (Why not in 'BeginPlay'?: Make sure subscription logic ran before broadcasting)
	//if (!bTriggeredOnce) {

	//	UE_LOG(LogTemp, Warning, TEXT("Broadcast Event!"));

	//	// Simple performance test (10.000 broadcasts)
	//	for (int i = 0; i < 10; ++i) {
	//		OnLightSwitchToggled.Broadcast(true);
	//	}

	//	bTriggeredOnce = true;
	//}
}
