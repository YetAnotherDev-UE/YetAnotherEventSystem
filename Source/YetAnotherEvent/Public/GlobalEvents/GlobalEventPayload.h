// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "GlobalEventPayload.generated.h"

USTRUCT(BlueprintType)
struct FGlobalEventPayload {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FGameplayTag Channel{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FGameplayTag EventTag{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	TObjectPtr<UObject> Sender{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FInstancedStruct Data{};
};
