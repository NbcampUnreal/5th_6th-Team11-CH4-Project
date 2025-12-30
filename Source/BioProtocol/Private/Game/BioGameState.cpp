// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/BioGameState.h"
#include "Net/UnrealNetwork.h"
#include "Game/BioGameMode.h"

ABioGameState::ABioGameState()
{
	CurrentPhase = EBioGamePhase::Day;
	PhaseTimeRemaining = 0;
	OxygenLevel = 100.0f;
}

void ABioGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioGameState, CurrentPhase);
	DOREPLIFETIME(ABioGameState, PhaseTimeRemaining);
	DOREPLIFETIME(ABioGameState, OxygenLevel);
	DOREPLIFETIME(ABioGameState, bCanEscape);
	DOREPLIFETIME(ABioGameState, MissionProgress);
	DOREPLIFETIME(ABioGameState, MaxMissionProgress);
}

void ABioGameState::RegisterTask()
{
	if (HasAuthority()) {
		MaxMissionProgress++;
	}
}

void ABioGameState::SetGamePhase(EBioGamePhase NewPhase)
{
	if (HasAuthority())
	{
		CurrentPhase = NewPhase;
		OnRep_Phase();
	}
}

void ABioGameState::AddMissionProgress(int32 Amount)
{
	if (HasAuthority())
	{
		MissionProgress += Amount;
		MissionProgress = FMath::Clamp(MissionProgress, 0, MaxMissionProgress);
		UE_LOG(LogTemp, Log, TEXT("%d/%d"), MissionProgress,MaxMissionProgress);

		if (MissionProgress >= MaxMissionProgress)
		{
			if (!bCanEscape)
			{
				bCanEscape = true;

				OnEscapeEnabled.Broadcast();

				UE_LOG(LogTemp, Warning, TEXT("[GameState] Mission Complete! Escape Enabled."));
			}
		}
	}
}

void ABioGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(CurrentPhase);
}

void ABioGameState::OnRep_MissionProgress()
{
	// 진행도가 변경될 때 UI 업데이트
}

void ABioGameState::OnRep_CanEscape()
{
	if (bCanEscape)
	{
		// 탈출이 가능할 때 UI 업데이트
		UE_LOG(LogTemp, Log, TEXT("CanEscape"));

	}
}
