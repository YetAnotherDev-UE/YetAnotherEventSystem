// Fill out your copyright notice in the Description page of Project Settings.

#include "LightSwitchActor.h"
#include "GameplayLightActor.h"

ALightSwitchActor::ALightSwitchActor() {
	PrimaryActorTick.bCanEverTick = true;
}

void ALightSwitchActor::Tick(float DeltaTime) { }

void ALightSwitchActor::TurnOnLight() {
	if (AttachedLight) 
		AttachedLight->SetLightIntensity(LightIntensity);
}























