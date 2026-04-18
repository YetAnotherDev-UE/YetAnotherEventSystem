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
	UEVENT(BlueprintCallable)
	TEvent<bool> OnLightSwitchToggled{};

	#pragma region UEVENT Generated Code (DO NOT TOUCH!)


	#pragma region Generated Blueprint Delegate for OnLightSwitchToggled
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLightSwitchToggledBP, bool, Param1);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLightSwitchToggledBP OnLightSwitchToggled_BP;
	private:
	uint8 _AutoBind_OnLightSwitchToggled = [this]() -> uint8 {
		OnLightSwitchToggled.SubscribeLambda([this](auto&&... args) {
			OnLightSwitchToggled_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	// Auto-Generated Blueprint Callables
	UFUNCTION(BlueprintCallable, Category = "Events|OnLightSwitchToggled", meta = (DisplayName = "Broadcast OnLightSwitchToggled"))
	void BP_Broadcast_OnLightSwitchToggled(bool Param1) {
		OnLightSwitchToggled.Broadcast(Param1);
	}

	UFUNCTION(BlueprintCallable, Category = "Events|OnLightSwitchToggled", meta = (DisplayName = "Sliced Broadcast OnLightSwitchToggled", AdvancedDisplay = "MaxExecutionsPerFrame"))
	void BP_SlicedBroadcast_OnLightSwitchToggled(bool Param1, int32 MaxExecutionsPerFrame = 10) {
		OnLightSwitchToggled.SlicedBroadcast(MaxExecutionsPerFrame, Param1);
	}

	#pragma endregion
	private:
	#pragma endregion
};
