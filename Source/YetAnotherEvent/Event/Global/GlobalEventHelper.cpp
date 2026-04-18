// Fill out your copyright notice in the Description page of Project Settings.

#include "GlobalEventHelper.h"

#include "GlobalEventSubsystem.h"

void UGlobalEventHelper::BroadcastGlobalEvent(const UObject* WorldContextObject, FGameplayTag Tag, UObject* Sender /*= nullptr*/, const FInstancedStruct& Payload /*= FInstancedStruct()*/) {
	UGlobalEventSubsystem* EventSystem = WorldContextObject->GetWorld()->GetSubsystem<UGlobalEventSubsystem>();
	if (!EventSystem) return;

	EventSystem->BroadcastByTag(Tag, Sender, Payload);
}
