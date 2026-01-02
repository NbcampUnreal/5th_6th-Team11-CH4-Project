// Fill out your copyright notice in the Description page of Project Settings.


#include "Session/LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, bIsReady);
	//DOREPLIFETIME(ALobbyPlayerState, Nickname);
	DOREPLIFETIME(ALobbyPlayerState, CurrentColorIndex);
}

//void ALobbyPlayerState::SetNickname(const FString& InName)
//{
//	if (HasAuthority())
//	{
//		Nickname = InName;
//
//		OnRep_Nickname();
//
//		SetPlayerName(InName);
//	}
//	else
//	{
//		ServerSetNickname(InName);
//	}
//}
//
//void ALobbyPlayerState::ServerSetNickname_Implementation(const FString& InName)
//{
//	SetNickname(InName);
//}

void ALobbyPlayerState::ServerToggleReady_Implementation()
{
	bIsReady = !bIsReady;
	OnRep_IsReady();
}

void ALobbyPlayerState::OnRep_IsReady()
{
	OnStatusChanged.Broadcast();
}

//void ALobbyPlayerState::OnRep_Nickname()
//{
//	OnStatusChanged.Broadcast();
//}

void ALobbyPlayerState::ServerSetColorIndex_Implementation(int32 NewIndex)
{
	if (CurrentColorIndex == NewIndex) return;

	if (IsColorAvailable(NewIndex))
	{
		CurrentColorIndex = NewIndex;
		OnRep_CurrentColorIndex();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Color %d is already taken!"), NewIndex);
	}
}

void ALobbyPlayerState::OnRep_CurrentColorIndex()
{
	OnStatusChanged.Broadcast();
}

bool ALobbyPlayerState::IsColorAvailable(int32 CheckIndex)
{
	UWorld* World = GetWorld();
	if (!World) return false;

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState) return false;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);

		if (LobbyPS && LobbyPS != this)
		{
			if (LobbyPS->CurrentColorIndex == CheckIndex)
			{
				return false;
			}
		}
	}

	return true;
}