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
	virtual void Tick(float DeltaTime) override;

public:
	UEVENT()
	TEvent<bool> OnLightSwitchToggled{};

	bool bTriggeredOnce{};

	#pragma region UEVENT Generated Code (DO NOT TOUCH!)


	#pragma region Generated Blueprint Delegate for OnLightSwitchToggled
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLightSwitchToggledBP, bool, Param1);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLightSwitchToggledBP OnLightSwitchToggled_BP;
	private:
	uint8 _AutoBind_OnLightSwitchToggled = [this]() -> uint8 {
		OnLightSwitchToggled.SubscribeLambda(this, [this](auto&&... args) {
			OnLightSwitchToggled_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion
	private:
	#pragma endregion
};
