// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "MissionObjectBase.generated.h"

class UStaticMeshComponent;
class ADXPlayerCharacter;
class AEquippableItem;

/**
 * 미션 오브젝트 베이스 클래스
 * 수리/상호작용이 필요한 오브젝트
 */
UCLASS()
class BIOPROTOCOL_API AMissionObjectBase : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	
	AMissionObjectBase();

protected:
	virtual void BeginPlay() override;

public:
	//==========================================
	// COMPONENTS
	//==========================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission Object")
	UStaticMeshComponent* ObjectMesh;

	//==========================================
	// MISSION PROPERTIES
	//==========================================

	/** 필요한 도구 ID (NAME_None이면 도구 불필요) */
	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|Requirements")
	FName RequiredToolID;

	/** 수리/완료 소요 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|Requirements")
	float CompletionTime;

	/** 완료 상태 */
	UPROPERTY(ReplicatedUsing = OnRep_IsCompleted, BlueprintReadOnly, Category = "Mission Object|State")
	bool bIsCompleted;

	/** 진행 중 상태 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission Object|State")
	bool bInProgress;

	/** 현재 작업 중인 플레이어 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission Object|State")
	ADXPlayerCharacter* CurrentWorker;

	/** 진행도 (0.0 ~ 1.0) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission Object|State")
	float Progress;

	//==========================================
	// VFX & AUDIO
	//==========================================

	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|VFX")
	UParticleSystem* WorkingParticle;

	UPROPERTY()
	UParticleSystemComponent* ActiveWorkingParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|Audio")
	USoundBase* WorkingSound;

	UPROPERTY()
	UAudioComponent* ActiveWorkingSound;

	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|VFX")
	UParticleSystem* CompletionParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|Audio")
	USoundBase* CompletionSound;

	//==========================================
	// TIMER
	//==========================================

	FTimerHandle WorkTimerHandle;

	//==========================================
	// FUNCTIONS
	//==========================================

	/** 작업 시작 */
	UFUNCTION(BlueprintCallable, Category = "Mission Object")
	void StartWork(ADXPlayerCharacter* Worker);

	/** 작업 취소 */
	UFUNCTION(BlueprintCallable, Category = "Mission Object")
	void CancelWork();

	/** 작업 완료 */
	UFUNCTION(BlueprintCallable, Category = "Mission Object")
	void CompleteWork();

	/** 진행도 업데이트 */
	void UpdateProgress(float DeltaTime);

	//==========================================
	// INTERACTION INTERFACE
	//==========================================

	virtual void BeginFocus_Implementation() override;
	virtual void EndFocus_Implementation() override;
	virtual void BeginInteract_Implementation() override;
	virtual void EndInteract_Implementation() override;
	virtual void Interact_Implementation(ADXPlayerCharacter* PlayerCharacter) override;
	virtual FInteractableData GetInteractableData_Implementation() const override;
	virtual bool CanInteract_Implementation(ADXPlayerCharacter* PlayerCharacter) const override;

	//==========================================
	// EVENTS
	//==========================================

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionCompleted, AMissionObjectBase*, MissionObject);
	UPROPERTY(BlueprintAssignable, Category = "Mission Object|Events")
	FOnMissionCompleted OnMissionCompleted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProgressChanged, float, NewProgress, float, MaxProgress);
	UPROPERTY(BlueprintAssignable, Category = "Mission Object|Events")
	FOnProgressChanged OnProgressChanged;

protected:
	/** 상호작용 데이터 */
	UPROPERTY(EditInstanceOnly, Category = "Mission Object")
	FInteractableData InstanceInteractableData;

	/** 작업 이펙트 시작 */
	void StartWorkEffects();

	/** 작업 이펙트 중지 */
	void StopWorkEffects();

	/** 플레이어가 필요한 도구를 가지고 있는지 확인 */
	bool HasRequiredTool(ADXPlayerCharacter* Player) const;

	//==========================================
	// SERVER RPC
	//==========================================

	UFUNCTION(Server, Reliable)
	void ServerStartWork(ADXPlayerCharacter* Worker);

	UFUNCTION(Server, Reliable)
	void ServerCancelWork();

	UFUNCTION(Server, Reliable)
	void ServerCompleteWork();

	//==========================================
	// REPLICATION
	//==========================================

	UFUNCTION()
	void OnRep_IsCompleted();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
