// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Event/EventSystem.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LightSwitchActor.generated.h"

UCLASS()
class YETANOTHEREVENT_API ALightSwitchActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ALightSwitchActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UEVENT() 
	TEvent<> OnLightSwitchTurnedOn_One{};
	#pragma region Generated Blueprint Delegate for OnLightSwitchTurnedOn_One
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLightSwitchTurnedOn_OneBP);
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "WARNING: Do not bind via the Details Panel for external actors (causes memory stacking). Use the 'Bind Event' node in BeginPlay instead."), Category = "Events")
	FOnLightSwitchTurnedOn_OneBP OnLightSwitchTurnedOn_One_BP;
	private:
	uint8 _AutoBind_OnLightSwitchTurnedOn_One = [this]() -> uint8 {
		OnLightSwitchTurnedOn_One.SubscribeLambda([this](auto&&... args) {
			OnLightSwitchTurnedOn_One_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		return 0;
	}();
	public:
	#pragma endregion

	UEVENT(Networkable)
	TEvent<int32> OnLightSwitchTurnedOn_Two{};
	#pragma region Generated Blueprint Delegate for OnLightSwitchTurnedOn_Two
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLightSwitchTurnedOn_TwoBP, int32, Param1);
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "WARNING: Do not bind via the Details Panel for external actors (causes memory stacking). Use the 'Bind Event' node in BeginPlay instead."), Category = "Events")
	FOnLightSwitchTurnedOn_TwoBP OnLightSwitchTurnedOn_Two_BP;
	private:
	uint8 _AutoBind_OnLightSwitchTurnedOn_Two = [this]() -> uint8 {
		OnLightSwitchTurnedOn_Two.SubscribeLambda([this](auto&&... args) {
			OnLightSwitchTurnedOn_Two_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		OnLightSwitchTurnedOn_Two.NetworkInterceptor = [this](auto&&... args) -> bool {
			if (this->HasAuthority()) {
				Multicast_OnLightSwitchTurnedOn_Two(Forward<decltype(args)>(args)...);
				return true;
			}
			return false;
		};
		return 0;
	}();
	public:
	// Auto-Generated NetMulticast RPC Wrapper
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnLightSwitchTurnedOn_Two(int32 Param1);
	virtual void Multicast_OnLightSwitchTurnedOn_Two_Implementation(int32 Param1) {
		OnLightSwitchTurnedOn_Two.Broadcast(Param1);
	}

	#pragma endregion

	UEVENT(ClientOnly)
	TEvent<bool, float> OnLightSwitchTurnedOn_Three{};
	#pragma region Generated Blueprint Delegate for OnLightSwitchTurnedOn_Three
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLightSwitchTurnedOn_ThreeBP, bool, Param1, float, Param2);
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "WARNING: Do not bind via the Details Panel for external actors (causes memory stacking). Use the 'Bind Event' node in BeginPlay instead."), Category = "Events")
	FOnLightSwitchTurnedOn_ThreeBP OnLightSwitchTurnedOn_Three_BP;
	private:
	uint8 _AutoBind_OnLightSwitchTurnedOn_Three = [this]() -> uint8 {
		OnLightSwitchTurnedOn_Three.SubscribeLambda([this](auto&&... args) {
			OnLightSwitchTurnedOn_Three_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		OnLightSwitchTurnedOn_Three.NetworkInterceptor = [this](auto&&... args) -> bool {
			if (this->HasAuthority()) {
				Client_OnLightSwitchTurnedOn_Three(Forward<decltype(args)>(args)...);
				return true;
			}
			return false;
		};
		return 0;
	}();
	public:
	// Auto-Generated Client RPC Wrapper
	UFUNCTION(Client, Reliable)
	void Client_OnLightSwitchTurnedOn_Three(bool Param1, float Param2);
	virtual void Client_OnLightSwitchTurnedOn_Three_Implementation(bool Param1, float Param2) {
		OnLightSwitchTurnedOn_Three.Broadcast(Param1, Param2);
	}

	#pragma endregion

	UEVENT(ServerOnly)
	TEvent<> OnLightSwitchTurnedOn_Four{};
	#pragma region Generated Blueprint Delegate for OnLightSwitchTurnedOn_Four
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLightSwitchTurnedOn_FourBP);
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "WARNING: Do not bind via the Details Panel for external actors (causes memory stacking). Use the 'Bind Event' node in BeginPlay instead."), Category = "Events")
	FOnLightSwitchTurnedOn_FourBP OnLightSwitchTurnedOn_Four_BP;
	private:
	uint8 _AutoBind_OnLightSwitchTurnedOn_Four = [this]() -> uint8 {
		OnLightSwitchTurnedOn_Four.SubscribeLambda([this](auto&&... args) {
			OnLightSwitchTurnedOn_Four_BP.Broadcast(Forward<decltype(args)>(args)...);
		});
		OnLightSwitchTurnedOn_Four.NetworkInterceptor = [this](auto&&... args) -> bool {
			if (this->HasAuthority()) {
				Server_OnLightSwitchTurnedOn_Four(Forward<decltype(args)>(args)...);
				return true;
			}
			return false;
		};
		return 0;
	}();
	public:
	// Auto-Generated Server RPC Wrapper
	UFUNCTION(Server, Reliable)
	void Server_OnLightSwitchTurnedOn_Four();
	virtual void Server_OnLightSwitchTurnedOn_Four_Implementation() {
		OnLightSwitchTurnedOn_Four.Broadcast();
	}

	#pragma endregion

private:
	bool bTriggeredOnce{};
};
