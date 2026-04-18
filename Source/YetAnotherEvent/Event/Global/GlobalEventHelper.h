// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GlobalEventHelper.generated.h"

UCLASS()
class YETANOTHEREVENT_API UGlobalEventHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (AutoCreateRefTerm = "Payload", WorldContext = "WorldContextObject"), Category = "Own|Global Events")
	static void BroadcastGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, UObject* Sender = nullptr, const FInstancedStruct& Payload = FInstancedStruct());
};
