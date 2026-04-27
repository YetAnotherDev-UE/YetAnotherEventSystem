// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "GlobalEventGate.generated.h"

USTRUCT(BlueprintType)
struct FGlobalEventGate {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FGameplayTagQuery EventConditionQuery;
};
