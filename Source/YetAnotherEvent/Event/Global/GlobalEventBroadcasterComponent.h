// Copyright (c) 2026 Core Memory Entertainment GbR. All rights reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StructUtils/PropertyBag.h"
#include "GlobalEventBroadcasterComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class YETANOTHEREVENT_API UGlobalEventBroadcasterComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UGlobalEventBroadcasterComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Own|Global Event Broadcaster")
	void TriggerEvents();

private:
	UPROPERTY(EditInstanceOnly, Category="Own|Global Event Broadcaster")
	FGameplayTagContainer TagsToTrigger{};

	UPROPERTY(EditInstanceOnly, Category="Own|Global Event Broadcaster")
	FGameplayTagContainer Channels{};

	UPROPERTY(EditInstanceOnly, Category = "Own|Global Event Broadcaster")
	FInstancedStruct PayloadData{};
};
