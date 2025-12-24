// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioPlayerState.h"
#include "Net/UnrealNetwork.h"

ABioPlayerState::ABioPlayerState()
{
	GameRole = EBioPlayerRole::None;
	Status = EBioPlayerStatus::Alive;
	MaxHP = 100.0f;
	CurrentHP = 100.0f;
	JailTimer = 0.0f;
}

void ABioPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioPlayerState, GameRole);
	DOREPLIFETIME(ABioPlayerState, CurrentHP);
	DOREPLIFETIME(ABioPlayerState, MaxHP);
	DOREPLIFETIME(ABioPlayerState, Status);
	DOREPLIFETIME(ABioPlayerState, JailTimer);
	DOREPLIFETIME(ABioPlayerState, ColorIndex);
	DOREPLIFETIME(ABioPlayerState, bIsTransformed);
}

void ABioPlayerState::ApplyDamage(float DamageAmount)
{
	if (!HasAuthority() || Status != EBioPlayerStatus::Alive) return;

	CurrentHP -= DamageAmount;

	if (CurrentHP <= 0.0f)
	{
		CurrentHP = 0.0f;
		SetPlayerStatus(EBioPlayerStatus::Jailed);
	}
}

void ABioPlayerState::SetPlayerStatus(EBioPlayerStatus NewStatus)
{
	Status = NewStatus;

	if (Status == EBioPlayerStatus::Jailed)
	{
		JailTimer = 60.0f;
	}
	else if (Status == EBioPlayerStatus::Alive)
	{
		CurrentHP = MaxHP * 0.3f;
		JailTimer = 0.0f;
	}
}

void ABioPlayerState::OnRep_GameRole()
{
	OnRoleChanged.Broadcast(GameRole);

	UE_LOG(LogTemp, Log, TEXT("OnRep_Role: Role received as %d"), (int32)GameRole);
}

void ABioPlayerState::OnRep_Status()
{

}

void ABioPlayerState::ServerSetTransform_Implementation(bool bNewState)
{
	bIsTransformed = bNewState;

	OnRep_IsTransformed();
}

void ABioPlayerState::OnRep_IsTransformed()
{
	OnTransformChanged.Broadcast(bIsTransformed);
}
