// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "GlobalEventSettings.generated.h"

UCLASS(Config=Game, defaultconfig, meta = (DisplayName = "Global Event Settings"))
class YETANOTHEREVENT_API UGlobalEventSettings : public UDeveloperSettings {
	GENERATED_BODY()
	
public:
	// Stored as a soft class path for now so runtime settings do not need a hard UMG dependency.
	UPROPERTY(Config, EditAnywhere, Category = "Debugging", meta = (MetaClass = "/Script/UMG.UserWidget"))
	FSoftClassPath EventDebuggerClass{};

	UPROPERTY(Config, EditAnywhere, Category = "Debugging")
	FString ToggleEventDebuggerCommand{ TEXT("Events.Debug") };
};
