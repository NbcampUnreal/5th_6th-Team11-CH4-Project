// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/BioGameState.h"
#include "Net/UnrealNetwork.h"

ABioGameState::ABioGameState()
{
	CurrentPhase = EBioGamePhase::Lobby;
	PhaseTimeRemaining = 0;
	OxygenLevel = 100.0f;
}

void ABioGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioGameState, CurrentPhase);
	DOREPLIFETIME(ABioGameState, PhaseTimeRemaining);
	DOREPLIFETIME(ABioGameState, OxygenLevel);
}

void ABioGameState::SetGamePhase(EBioGamePhase NewPhase)
{
	if (HasAuthority())
	{
		CurrentPhase = NewPhase;
		OnRep_Phase();
	}
}

void ABioGameState::OnRep_Phase()
{
	OnPhaseChanged.Broadcast(CurrentPhase);
}
