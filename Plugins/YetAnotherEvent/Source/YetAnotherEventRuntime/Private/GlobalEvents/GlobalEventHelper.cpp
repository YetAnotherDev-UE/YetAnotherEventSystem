// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEvents/GlobalEventHelper.h"

#include "GlobalEvents/GlobalEventSubsystem.h"
#include "Engine/GameInstance.h"
#include "YetAnotherEventLog.h"
#include "Engine/World.h"

UGlobalEventSubsystem* UGlobalEventHelper::GetEventSubsystem(const UObject* WorldContextObject, const TCHAR* OperationName) {
	if (!WorldContextObject) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("%s failed: WorldContextObject is null."), OperationName);
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("%s failed: WorldContextObject '%s' has no valid world."), OperationName, *GetNameSafe(WorldContextObject));
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("%s failed: world '%s' has no valid game instance."), OperationName, *GetNameSafe(World));
		return nullptr;
	}

	UGlobalEventSubsystem* EventSystem = GameInstance->GetSubsystem<UGlobalEventSubsystem>();
	if (!EventSystem) {
		UE_LOG(LogYetAnotherEvent, Warning, TEXT("%s failed: GlobalEventSubsystem is unavailable."), OperationName);
	}

	return EventSystem;
}

void UGlobalEventHelper::BroadcastGlobalEvent(const UObject* WorldContextObject, const FGlobalEventPayload& Payload, bool bPropagateToParents /*= true*/) {
	UGlobalEventSubsystem* EventSystem = GetEventSubsystem(WorldContextObject, TEXT("BroadcastGlobalEvent"));
	if (!EventSystem) {
		return;
	}

	EventSystem->BroadcastByTag(Payload, bPropagateToParents);
}

void UGlobalEventHelper::UnsubscribeFromGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, FEventHandle Handle) {
	if (!Tag.IsValid() || !Handle.IsValid()) {
		return;
	}

	UGlobalEventSubsystem* EventSystem = GetEventSubsystem(WorldContextObject, TEXT("UnsubscribeFromGlobalEvent"));
	if (!EventSystem) {
		return;
	}

	EventSystem->Unsubscribe(Tag, Handle);
}
