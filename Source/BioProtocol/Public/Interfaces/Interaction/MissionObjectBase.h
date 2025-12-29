// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "MissionObjectBase.generated.h"

class UStaticMeshComponent;
class AStaffCharacter;
class AEquippableItem;

/**
 * �̼� ������Ʈ ���̽� Ŭ����
 * ����/��ȣ�ۿ��� �ʿ��� ������Ʈ
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

	/** �ʿ��� ���� ID (NAME_None�̸� ���� ���ʿ�) */
	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|Requirements")
	FName RequiredToolID;

	/** ����/�Ϸ� �ҿ� �ð� (��) */
	UPROPERTY(EditDefaultsOnly, Category = "Mission Object|Requirements")
	float CompletionTime;

	/** �Ϸ� ���� */
	UPROPERTY(ReplicatedUsing = OnRep_IsCompleted, BlueprintReadOnly, Category = "Mission Object|State")
	bool bIsCompleted;

	/** ���� �� ���� */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission Object|State")
	bool bInProgress;

	/** ���� �۾� ���� �÷��̾� */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mission Object|State")
	AStaffCharacter* CurrentWorker;

	/** ���൵ (0.0 ~ 1.0) */
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

	/** �۾� ���� */
	UFUNCTION(BlueprintCallable, Category = "Mission Object")

	void StartWork(AStaffCharacter* Worker);

	/** �۾� ��� */
	UFUNCTION(BlueprintCallable, Category = "Mission Object")
	virtual void CancelWork();

	/** �۾� �Ϸ� */
	UFUNCTION(BlueprintCallable, Category = "Mission Object")
	virtual void CompleteWork();

	/** ���൵ ������Ʈ */
	virtual void UpdateProgress(float DeltaTime);

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
	/** ��ȣ�ۿ� ������ */
	UPROPERTY(EditInstanceOnly, Category = "Mission Object")
	FInteractableData InstanceInteractableData;

	/** �۾� ����Ʈ ���� */
	void StartWorkEffects();

	/** �۾� ����Ʈ ���� */
	void StopWorkEffects();

	/** �÷��̾ �ʿ��� ������ ������ �ִ��� Ȯ�� */
	bool HasRequiredTool(AStaffCharacter* Player) const;

	//==========================================
	// SERVER RPC
	//==========================================

	UFUNCTION(Server, Reliable)
	void ServerStartWork(AStaffCharacter* Worker);

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
