// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventHelper.h"

#include "GlobalEventSubsystem.h"

void UGlobalEventHelper::BroadcastGlobalEvent(const UObject* WorldContextObject, const FGlobalEventPayload& Payload, bool bPropagateToParents /*= true*/) {
	if (!WorldContextObject) return;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) return;
	UGlobalEventSubsystem* EventSystem = GameInstance->GetSubsystem<UGlobalEventSubsystem>();
	if (!EventSystem) return;
	
	EventSystem->BroadcastByTag(Payload, bPropagateToParents);
}

void UGlobalEventHelper::UnsubscribeFromGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, FEventHandle Handle) {	
	if (!Handle.IsValid()) return;

	if (!WorldContextObject) return;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) return;
	UGlobalEventSubsystem* EventSystem = GameInstance->GetSubsystem<UGlobalEventSubsystem>();
	if (!EventSystem) return;

	EventSystem->Unsubscribe(Tag, Handle);
}
