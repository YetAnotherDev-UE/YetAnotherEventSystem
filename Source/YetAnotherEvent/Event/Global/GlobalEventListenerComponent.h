// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GlobalEventGate.h"
#include "../EventSystem.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GlobalEventListenerComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class YETANOTHEREVENT_API UGlobalEventListenerComponent : public UActorComponent {
	GENERATED_BODY()

public:	
	UGlobalEventListenerComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:	
	UEVENT()
	TEvent<FGameplayTag, UObject*, const FInstancedStruct&> OnReceivedGlobalEvent{};

	UEVENT()
	TEvent<> OnPassedGlobalEventGate{};

private:
	UPROPERTY(EditAnywhere, Category = "Events")
	FGlobalEventGate GlobalEventGate{};

	UPROPERTY(EditAnywhere, Category = "Events")
	bool bResetGateWhenPassed{true};

	FGameplayTagContainer ReceivedEvents{};

	TMap<FGameplayTag, FEventHandle> ActiveHandles{};

	#pragma region UEVENT Generated Code (DO NOT TOUCH!)


	#pragma region Generated Blueprint Delegate for OnReceivedGlobalEvent
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnReceivedGlobalEventBP, FGameplayTag, Param1, UObject*, Param2, const FInstancedStruct&, Param3);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnReceivedGlobalEventBP OnReceivedGlobalEvent_BP;
	private:
	uint8 _AutoBind_OnReceivedGlobalEvent = [this]() -> uint8 {
		OnReceivedGlobalEvent.SubscribeLambda(this, [this](auto&&... args) {
			OnReceivedGlobalEvent_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion

	#pragma region Generated Blueprint Delegate for OnPassedGlobalEventGate
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPassedGlobalEventGateBP);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPassedGlobalEventGateBP OnPassedGlobalEventGate_BP;
	private:
	uint8 _AutoBind_OnPassedGlobalEventGate = [this]() -> uint8 {
		OnPassedGlobalEventGate.SubscribeLambda(this, [this](auto&&... args) {
			OnPassedGlobalEventGate_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion
	private:
	#pragma endregion
};
