// Fill out your copyright notice in the Description page of Project Settings.

#include "GameplayLightActor.h"
#include "Components/LightComponent.h"
#include "LightSwitchActor.h"

AGameplayLightActor::AGameplayLightActor() {
	PrimaryActorTick.bCanEverTick = false;
}

void AGameplayLightActor::BeginPlay() {
	Super::BeginPlay();	
	
	AttachedLight = FindComponentByClass<ULightComponent>();
	if (AttachedLight) {
		AttachedLight->SetVisibility(false);
	}

	if (MyLightSwitch) {
		MyLightSwitch->OnLightSwitchTurnedOn_One.SubscribeUObject(this, &AGameplayLightActor::PrintOne,   1);
		MyLightSwitch->OnLightSwitchTurnedOn_One.SubscribeUObject(this, &AGameplayLightActor::PrintTwo,   2);
		MyLightSwitch->OnLightSwitchTurnedOn_One.SubscribeUObject(this, &AGameplayLightActor::PrintThree, 3);
	}
}

void AGameplayLightActor::PrintOne() {
	UE_LOG(LogTemp, Warning, TEXT("1!"));
}

void AGameplayLightActor::PrintTwo() {
	UE_LOG(LogTemp, Warning, TEXT("2!"));
}

void AGameplayLightActor::PrintThree() {
	UE_LOG(LogTemp, Warning, TEXT("3!"));
}

void AGameplayLightActor::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
}

void AGameplayLightActor::EndPlay(EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
}




















