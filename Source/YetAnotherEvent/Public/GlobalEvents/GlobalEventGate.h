// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GlobalEventGate.generated.h"

USTRUCT(BlueprintType)
struct FGlobalEventGate {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Events|Gate")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Global Events|Gate", meta = (EditCondition = "bEnabled"))
	FGameplayTagQuery EventConditionQuery{};

	bool HasCondition() const {
		return !EventConditionQuery.IsEmpty();
	}

	bool ShouldEvaluate() const {
		return bEnabled && HasCondition();
	}
};
