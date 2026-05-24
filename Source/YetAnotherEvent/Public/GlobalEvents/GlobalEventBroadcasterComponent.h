// Copyright (c) 2026 Core Memory Entertainment GbR. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "GlobalEventBroadcasterComponent.generated.h"

USTRUCT(BlueprintType)
struct FGlobalEventBroadcastEntry {
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Global Events|Broadcast")
	FGameplayTag EventTag{};

	// Optional. If empty, the event is broadcast once without a channel.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Global Events|Broadcast")
	FGameplayTagContainer Channels{};

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Global Events|Broadcast")
	FInstancedStruct PayloadData{};

	// When true, listeners subscribed to parent tags can also receive this event.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Global Events|Broadcast|Advanced")
	bool bPropagateToParents = true;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class YETANOTHEREVENT_API UGlobalEventBroadcasterComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UGlobalEventBroadcasterComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(BlueprintCallable, Category = "Global Events|Broadcaster")
	void TriggerEvents();

private:
	void BroadcastEntry(const FGlobalEventBroadcastEntry& Entry, UObject* SenderObject) const;

private:
	// Each entry can have its own event tag, channels, payload, and propagation behavior.
	UPROPERTY(EditInstanceOnly, Category = "Global Events|Broadcaster", meta = (TitleProperty = "EventTag"))
	TArray<FGlobalEventBroadcastEntry> EventsToBroadcast{};
};
