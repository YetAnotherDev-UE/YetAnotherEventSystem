// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "CoreMinimal.h"
#include "GlobalEventPayload.generated.h"

USTRUCT(BlueprintType)
struct FGlobalEventPayload {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FGameplayTag Channel{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FGameplayTag EventTag{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	UObject* Sender{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	FInstancedStruct Data{};
};
