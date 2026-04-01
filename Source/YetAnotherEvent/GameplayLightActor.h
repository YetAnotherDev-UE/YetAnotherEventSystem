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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

private:
	void PrintOne();
	void PrintTwo();
	void PrintThree();

private:
	FEventHandle m_onLightSwitchTurnedOnHandle{};

	UPROPERTY()
	TObjectPtr<ULightComponent> AttachedLight{ nullptr };

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta = (DisplayName = "My Light Switch", AllowPrivateAccess = true), Category = "Own | Gameplay Light")
	TObjectPtr<ALightSwitchActor> MyLightSwitch{ nullptr };
};
