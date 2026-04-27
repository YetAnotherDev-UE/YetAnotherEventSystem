// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayLightActor.h"
#include "Components/LightComponent.h"
#include "LightSwitchActor.h"

AGameplayLightActor::AGameplayLightActor() {
	PrimaryActorTick.bCanEverTick = false;
}

void AGameplayLightActor::BeginPlay() {
	Super::BeginPlay();
}

void AGameplayLightActor::DoSomething(bool Value) {

}

















