// Fill out your copyright notice in the Description page of Project Settings.

#include "EventTriggerBox.h"

#include "Components/BillboardComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GlobalEventHelper.h"

AEventTriggerBox::AEventTriggerBox() {
	FilterClass = ACharacter::StaticClass();
	
	if (UBoxComponent* BoxComponent = Cast<UBoxComponent>(GetCollisionComponent())) {
		BoxComponent->SetBoxExtent(FVector(100.0f));
		BoxComponent->ShapeColor = FColor{ 0, 255, 236, 180 };
		BoxComponent->SetLineThickness(3.0f);
	}

#if WITH_EDITORONLY_DATA
	if (UBillboardComponent* SpriteComp = GetSpriteComponent()) {
		static ConstructorHelpers::FObjectFinder<UTexture2D> CustomEventIcon(TEXT("/Game/Textures/T_EventIcon.T_EventIcon"));
		if (CustomEventIcon.Succeeded()) {
			SpriteComp->SetSprite(CustomEventIcon.Object);
		}
	}
#endif
}

void AEventTriggerBox::BeginPlay() {
	Super::BeginPlay();

	if (UPrimitiveComponent* CollisionComp = GetCollisionComponent()) {
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AEventTriggerBox::OnOverlapBegin);
	}
}

void AEventTriggerBox::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (OtherActor && OtherActor != this) {
		// Check filter
		if (!FilterClass || OtherActor->IsA(FilterClass)) {
			TriggerAssignedEvents();
		}
	}
}

void AEventTriggerBox::TriggerAssignedEvents() {
	for (const FGameplayTag& EventTag : TagsToTrigger) {
		UGlobalEventHelper::BroadcastGlobalEvent(this, EventTag);
	}
}
