// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Game/BioProtocolTypes.h"
#include "BioGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGamePhaseChanged, EBioGamePhase, NewPhase);

UCLASS()
class BIOPROTOCOL_API ABioGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABioGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_Phase, BlueprintReadOnly, Category = "Bio|GameRule")
	EBioGamePhase CurrentPhase;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|GameRule")
	int32 PhaseTimeRemaining;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Bio|Sabotage")
	float OxygenLevel;

	UPROPERTY(BlueprintAssignable)
	FOnGamePhaseChanged OnPhaseChanged;

	void SetGamePhase(EBioGamePhase NewPhase);

protected:
	UFUNCTION()
	void OnRep_Phase();
};