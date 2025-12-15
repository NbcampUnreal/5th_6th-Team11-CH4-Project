// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
}

void ALobbyPlayerState::ServerToggleReady_Implementation()
{
	bIsReady = !bIsReady;

	OnStatusChanged.Broadcast();
}

void ALobbyPlayerState::OnRep_IsReady()
{
	OnStatusChanged.Broadcast();
}
