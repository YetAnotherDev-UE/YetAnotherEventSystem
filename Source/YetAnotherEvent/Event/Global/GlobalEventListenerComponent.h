// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:	
	UEVENT()
	TEvent<FGameplayTag, UObject*, const FInstancedStruct&> OnGlobalEventFired{};

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true), Category = "Events")
	FGameplayTagContainer TagsToListenFor;

	#pragma region UEVENT Generated Code (DO NOT TOUCH!)


	#pragma region Generated Blueprint Delegate for OnGlobalEventFired
	public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGlobalEventFiredBP, FGameplayTag, Param1, UObject*, Param2, const FInstancedStruct&, Param3);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGlobalEventFiredBP OnGlobalEventFired_BP;
	private:
	uint8 _AutoBind_OnGlobalEventFired = [this]() -> uint8 {
		OnGlobalEventFired.SubscribeLambda([this](auto&&... args) {
			OnGlobalEventFired_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion
	private:
	#pragma endregion
};
