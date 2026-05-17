// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Event/EventSystem.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayLightActor.generated.h"

class ULightComponent;
class ALightSwitchActor;

UCLASS()
class YETANOTHEREVENT_API AGameplayLightActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AGameplayLightActor();
	void SetLightIntensity(float InIntensity);

protected:
	virtual void BeginPlay() override;
};
