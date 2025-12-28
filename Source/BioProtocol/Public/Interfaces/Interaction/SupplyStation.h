// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BioProtocol/Public/Interfaces/Interaction/MissionObjectBase.h" 
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "SupplyStation.generated.h"

class UStaticMeshComponent;
class AStaffCharacter;
class AEquippableTool_Welder;

/**
 * 보급소 - 용접기의 내구도를 충전하는 상호작용 오브젝트
 */

UCLASS()
class BIOPROTOCOL_API ASupplyStation : public AMissionObjectBase
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASupplyStation();

protected:
	virtual void BeginPlay() override;

public:
	//==========================================
	// COMPONENTS
	//==========================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Supply Station")
	UStaticMeshComponent* StationMesh;

	//==========================================
	// PROPERTIES
	//==========================================

	/** 충전 소요 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Supply Station|Settings")
	float RechargeTime;

	/** 초당 충전량 (0이면 즉시 완충) */
	UPROPERTY(EditDefaultsOnly, Category = "Supply Station|Settings")
	float RechargeRatePerSecond;

	/** 충전 중인지 여부 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Supply Station|State")
	bool bIsCharging;

	/** 현재 충전 중인 플레이어 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Supply Station|State")
	AStaffCharacter* ChargingPlayer;

	//==========================================
	// VFX & AUDIO
	//==========================================

	UPROPERTY(EditDefaultsOnly, Category = "Supply Station|VFX")
	UParticleSystem* ChargingParticle;

	UPROPERTY()
	UParticleSystemComponent* ActiveChargingParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Supply Station|Audio")
	USoundBase* ChargingSound;

	UPROPERTY()
	UAudioComponent* ActiveChargingSound;

	//==========================================
	// TIMER
	//==========================================

	FTimerHandle ChargingTimerHandle;

	//==========================================
	// FUNCTIONS
	//==========================================

	/** 충전 시작 */
	UFUNCTION(BlueprintCallable, Category = "Supply Station")
	void StartCharging(AStaffCharacter* Player);

	/** 충전 완료 */
	UFUNCTION(BlueprintCallable, Category = "Supply Station")
	void CompleteCharging();

	/** 충전 취소 */
	UFUNCTION(BlueprintCallable, Category = "Supply Station")
	void CancelCharging();

	//==========================================
	// INTERACTION INTERFACE
	//==========================================

	virtual void BeginFocus_Implementation() override;
	virtual void EndFocus_Implementation() override;
	virtual void BeginInteract_Implementation() override;
	virtual void EndInteract_Implementation() override;
	virtual void Interact_Implementation(AStaffCharacter* PlayerCharacter) override;
	virtual FInteractableData GetInteractableData_Implementation() const override;
	virtual bool CanInteract_Implementation(AStaffCharacter* PlayerCharacter) const override;

protected:
	/** 충전 이펙트 시작 */
	void StartChargingEffects();

	/** 충전 이펙트 중지 */
	void StopChargingEffects();

	/** 충전 진행 (Tick처럼 동작) */
	void ProcessCharging();

	//==========================================
	// SERVER RPC
	//==========================================

	UFUNCTION(Server, Reliable)
	void ServerStartCharging(AStaffCharacter* Player);

	UFUNCTION(Server, Reliable)
	void ServerCompleteCharging();

	UFUNCTION(Server, Reliable)
	void ServerCancelCharging();

	//==========================================
	// REPLICATION
	//==========================================

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

};
