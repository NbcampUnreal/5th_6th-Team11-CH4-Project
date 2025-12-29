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

	void RegisterTask();

	void SetGamePhase(EBioGamePhase NewPhase);

	// 임무 진행도
	UPROPERTY(ReplicatedUsing = OnRep_MissionProgress, BlueprintReadOnly, Category = "Bio|GameRule")
	int32 MissionProgress;

	// 임무 진행도를 다 채웠을 시 탈출 포드를 이용할 수 있게 할 변수
	UPROPERTY(ReplicatedUsing = OnRep_CanEscape, BlueprintReadOnly, Category = "Bio|GameRule")
	bool bCanEscape = false;

	void AddMissionProgress(int32 Amount);

protected:
	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_MissionProgress();

	UFUNCTION()
	void OnRep_CanEscape();

	UPROPERTY(BlueprintReadOnly, Category = "Bio|GameRule")
	int32 MaxMissionProgress = 0;


};