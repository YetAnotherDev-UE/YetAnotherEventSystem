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

public:
	// UEVENT() 
	TEvent<> OnLightSwitchTurnedOn{};
	#pragma region Generated Blueprint Delegate for OnLightSwitchTurnedOn
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLightSwitchTurnedOnBP);
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "WARNING: Do not bind via the Details Panel for external actors (causes memory stacking). Use the 'Bind Event' node in BeginPlay instead."), Category = "Events")
	FOnLightSwitchTurnedOnBP OnLightSwitchTurnedOn_BP;
	private:
	uint8 _AutoBind_OnLightSwitchTurnedOn = [this]() -> uint8 {
		OnLightSwitchTurnedOn.SubscribeLambda([this](auto&&... args) {
			OnLightSwitchTurnedOn_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	bool bTriggeredOnce{};
};
