// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Event/EventSystem.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LightSwitchActor.generated.h"

UCLASS()
class YETANOTHEREVENT_API ALightSwitchActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ALightSwitchActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:

	TEvent<> OnLightSwitchTurnedOn_One{};
	
	TEvent<bool> OnLightSwitchTurnedOn_Two{};
	
	TEvent<int32> OnLightSwitchTurnedOn_Three{};

	TEvent<bool, float> OnLightSwitchTurnedOn_Four{};
		
	TEvent<> OnLightSwitchTurnedOn_Five{};

private:
	bool bTriggeredOnce{};
};
