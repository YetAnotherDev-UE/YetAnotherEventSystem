// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "EventTriggerBox.generated.h"

class UPrimitiveComponent;
class AActor;

UCLASS()
class YETANOTHEREVENT_API AEventTriggerBox : public ATriggerBox {
	GENERATED_BODY()
	
public:
	AEventTriggerBox();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TriggerAssignedEvents();

private:
	UPROPERTY(EditInstanceOnly, Category="Own|Event Trigger Box")
	FGameplayTagContainer TagsToTrigger{};

	UPROPERTY(EditInstanceOnly, Category="Own|Event Trigger Box")
	TSubclassOf<AActor> FilterClass{};
};
