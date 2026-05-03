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
	UEVENT(ParamNames = ("Param1", "Param2", "Param3", "Param4", "Param5"))
	TEvent<int64, int32, int, FVector, bool> OnLightSwitchToggled_Old{};

	// Uses automatic type deduction
	UEVENT()
	TEvent<int64, int32, int, FVector, bool> OnLightSwitchToggled_New{};

	// Uses custom parameter names
	UEVENT(ParamNames = ("Amount", "Score", "Health", "PlayerLocation", "SelfDestruct"))
	TEvent<int64, int32, int, FVector, bool> OnLightSwitchToggled_New_Two{};

	bool bTriggeredOnce{};

	#pragma region UEVENT Generated Code (DO NOT TOUCH!)


	#pragma region Generated Blueprint Delegate for OnLightSwitchToggled_Old
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnLightSwitchToggled_OldBP, int64, Param1, int32, Param2, int, Param3, FVector, Param4, bool, Param5);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLightSwitchToggled_OldBP OnLightSwitchToggled_Old_BP;
	private:
	uint8 _AutoBind_OnLightSwitchToggled_Old = [this]() -> uint8 {
		OnLightSwitchToggled_Old.SubscribeLambda(this, [this](auto&&... args) {
			OnLightSwitchToggled_Old_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion

	#pragma region Generated Blueprint Delegate for OnLightSwitchToggled_New
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnLightSwitchToggled_NewBP, int64, Int64, int32, Int1, int, Int2, FVector, Location, bool, Bool);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLightSwitchToggled_NewBP OnLightSwitchToggled_New_BP;
	private:
	uint8 _AutoBind_OnLightSwitchToggled_New = [this]() -> uint8 {
		OnLightSwitchToggled_New.SubscribeLambda(this, [this](auto&&... args) {
			OnLightSwitchToggled_New_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion

	#pragma region Generated Blueprint Delegate for OnLightSwitchToggled_New_Two
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnLightSwitchToggled_New_TwoBP, int64, Amount, int32, Score, int, Health, FVector, PlayerLocation, bool, SelfDestruct);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnLightSwitchToggled_New_TwoBP OnLightSwitchToggled_New_Two_BP;
	private:
	uint8 _AutoBind_OnLightSwitchToggled_New_Two = [this]() -> uint8 {
		OnLightSwitchToggled_New_Two.SubscribeLambda(this, [this](auto&&... args) {
			OnLightSwitchToggled_New_Two_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion
	private:
	#pragma endregion
};











