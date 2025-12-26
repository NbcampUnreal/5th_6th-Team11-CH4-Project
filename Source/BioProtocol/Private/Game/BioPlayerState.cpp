// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioPlayerState.h"
#include "Net/UnrealNetwork.h"

ABioPlayerState::ABioPlayerState()
{
	GameRole = EBioPlayerRole::None;
	ColorIndex = -1;
}

void ABioPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		TryInitEOSPlayerName();
	}
}

void ABioPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioPlayerState, GameRole);
	DOREPLIFETIME(ABioPlayerState, ColorIndex);
	DOREPLIFETIME(ABioPlayerState, EOSPlayerName);
}

void ABioPlayerState::TryInitEOSPlayerName()
{
    if (!EOSPlayerName.IsEmpty()) return;

    if (!GetUniqueId().IsValid())
    {
        FTimerHandle RetryTimer;
        GetWorldTimerManager().SetTimer(RetryTimer, this, &ABioPlayerState::TryInitEOSPlayerName, 0.2f, false);
        return;
    }

    FString Raw = GetUniqueId()->ToString();
    FString Left, Right;
    if (Raw.Split(TEXT("|"), &Left, &Right))
    {
        EOSPlayerName = Right;
    }
    else
    {
        EOSPlayerName = Raw;
    }

    ForceNetUpdate();
}

void ABioPlayerState::Server_SetEOSPlayerName_Implementation(const FString& InName)
{
    EOSPlayerName = InName;
}

void ABioPlayerState::OnRep_GameRole()
{
	OnRoleChanged.Broadcast(GameRole);

	UE_LOG(LogTemp, Log, TEXT("OnRep_Role: Role received as %d"), (int32)GameRole);
}

