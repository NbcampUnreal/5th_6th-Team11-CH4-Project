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

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Appearance")
	int32 ColorIndex;

	UPROPERTY(BlueprintAssignable, Category = "Bio|Event")
	FOnPlayerRoleChanged OnRoleChanged;

protected:
	UFUNCTION()
	void OnRep_GameRole();

};