// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioPlayerState.h"
#include "Net/UnrealNetwork.h"

ABioPlayerState::ABioPlayerState()
{
	GameRole = EBioPlayerRole::None;
	ColorIndex = -1;
}

void ABioPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioPlayerState, GameRole);
	DOREPLIFETIME(ABioPlayerState, ColorIndex);
}


void ABioPlayerState::OnRep_GameRole()
{
	OnRoleChanged.Broadcast(GameRole);

	UE_LOG(LogTemp, Log, TEXT("OnRep_Role: Role received as %d"), (int32)GameRole);
}