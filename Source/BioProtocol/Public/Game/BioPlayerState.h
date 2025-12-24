// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Game/BioProtocolTypes.h"
#include "BioPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRoleChanged, EBioPlayerRole, NewRole);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTransformChanged, bool, bIsTransformed);

UCLASS()
class BIOPROTOCOL_API ABioPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ABioPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_GameRole, BlueprintReadOnly, Category = "Bio|Role")
	EBioPlayerRole GameRole;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Stat")
	float CurrentHP;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Stat")
	float MaxHP;

	UPROPERTY(ReplicatedUsing = OnRep_Status, BlueprintReadOnly, Category = "Bio|Status")
	EBioPlayerStatus Status;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Status")
	float JailTimer;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Appearance")
	int32 ColorIndex;

	UPROPERTY(ReplicatedUsing = OnRep_IsTransformed, BlueprintReadOnly, Category = "Bio|State")
	bool bIsTransformed;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerTransformChanged OnTransformChanged;

	UPROPERTY(BlueprintAssignable, Category = "Bio|Event")
	FOnPlayerRoleChanged OnRoleChanged;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetTransform(bool bNewState);

	void ApplyDamage(float DamageAmount);

	void SetPlayerStatus(EBioPlayerStatus NewStatus);



protected:
	UFUNCTION()
	void OnRep_GameRole();

	UFUNCTION()
	void OnRep_Status();

	UFUNCTION()
	void OnRep_IsTransformed();
};