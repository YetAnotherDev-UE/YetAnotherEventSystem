// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventHelper.h"

#include "GlobalEventSubsystem.h"

void UGlobalEventHelper::BroadcastGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, UObject* Sender /*= nullptr*/, const FInstancedStruct& Payload /*= FInstancedStruct()*/) {
	if (!WorldContextObject) return;
	UWorld* World = WorldContextObject->GetWorld();
	if (!World) return;
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) return;
	UGlobalEventSubsystem* EventSystem = GameInstance->GetSubsystem<UGlobalEventSubsystem>();
	if (!EventSystem) return;
	
	EventSystem->BroadcastByTag(Tag, Sender, Payload);
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
