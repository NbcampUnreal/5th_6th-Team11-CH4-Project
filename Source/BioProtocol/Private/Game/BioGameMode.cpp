// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioGameMode.h"
#include "Game/BioGameState.h"
#include "Game/BioPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

ABioGameMode::ABioGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = ABioGameState::StaticClass();
	PlayerStateClass = ABioPlayerState::StaticClass();
}

void ABioGameMode::StartPlay()
{
	Super::StartPlay();

	BioGameState = GetGameState<ABioGameState>();

	AssignRoles();

	if (BioGameState)
	{
		BioGameState->SetGamePhase(EBioGamePhase::Day);
		BioGameState->PhaseTimeRemaining = DayDuration;
	}

	JailLocation = FVector(0, 0, 0);
}

void ABioGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
}

void ABioGameMode::AssignRoles()
{
	if (!GameState) return;

	TArray<APlayerState*> AllPlayers = GameState->PlayerArray;
	int32 PlayerCount = AllPlayers.Num();

	if (PlayerCount == 0) return;

	int32 TargetCleanerNum = 1;

	if (PlayerCount >= 5)
	{
		TargetCleanerNum = 2;
	}

	UE_LOG(LogTemp, Log, TEXT("Total Players: %d, Cleaner Count: %d"), PlayerCount, TargetCleanerNum);

	int32 LastIndex = PlayerCount - 1;
	for (int32 i = 0; i <= LastIndex; ++i)
	{
		int32 RandomIndex = FMath::RandRange(i, LastIndex);
		if (i != RandomIndex)
		{
			AllPlayers.Swap(i, RandomIndex);
		}
	}

	for (int32 i = 0; i < PlayerCount; ++i)
	{
		ABioPlayerState* BioPS = Cast<ABioPlayerState>(AllPlayers[i]);
		if (!BioPS) continue;

		if (i < TargetCleanerNum)
		{
			BioPS->GameRole = EBioPlayerRole::Cleaner;
			BioPS->MaxHP = 100.0f;
			BioPS->CurrentHP = BioPS->MaxHP;

			UE_LOG(LogTemp, Log, TEXT("-> Player [%s] assigned as CLEANER"), *BioPS->GetPlayerName());
		}
		else
		{
			BioPS->GameRole = EBioPlayerRole::Staff;
			BioPS->MaxHP = 100.0f;
			BioPS->CurrentHP = BioPS->MaxHP;

			UE_LOG(LogTemp, Log, TEXT("-> Player [%s] assigned as STAFF"), *BioPS->GetPlayerName());
		}
	}
}

void ABioGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!BioGameState) return;

	TimeCounter += DeltaSeconds;
	if (TimeCounter >= 1.0f)
	{
		TimeCounter = 0.0f;

		if (BioGameState->PhaseTimeRemaining > 0)
		{
			BioGameState->PhaseTimeRemaining--;
		}
		else
		{
			if (BioGameState->CurrentPhase == EBioGamePhase::Day)
			{
				BioGameState->SetGamePhase(EBioGamePhase::Night);
				BioGameState->PhaseTimeRemaining = NightDuration;

				UE_LOG(LogTemp, Warning, TEXT("NIGHT PHASE STARTED!"));
			}
			else if (BioGameState->CurrentPhase == EBioGamePhase::Night)
			{
				BioGameState->SetGamePhase(EBioGamePhase::Day);
				BioGameState->PhaseTimeRemaining = DayDuration;

				UE_LOG(LogTemp, Warning, TEXT("DAY PHASE STARTED!"));
			}
		}
	}

	UpdateJailTimers(DeltaSeconds);
}

void ABioGameMode::UpdateJailTimers(float DeltaTime)
{
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ABioPlayerState* BioPS = Cast<ABioPlayerState>(PS);
		if (BioPS && BioPS->Status == EBioPlayerStatus::Jailed)
		{
			BioPS->JailTimer -= DeltaTime;

			if (BioPS->JailTimer <= 0.0f)
			{
				BioPS->SetPlayerStatus(EBioPlayerStatus::Dead);

				APawn* Pawn = BioPS->GetPawn();
				if (Pawn)
				{
					Pawn->Destroy();
				}

				UE_LOG(LogTemp, Log, TEXT("%s has been Incinerated."), *BioPS->GetPlayerName());
				CheckWinConditions();
			}
		}
	}
}

void ABioGameMode::SendPlayerToJail(AController* PlayerToJail)
{
	if (!PlayerToJail) return;

	APawn* Pawn = PlayerToJail->GetPawn();
	if (Pawn)
	{
		Pawn->SetActorLocation(JailLocation);

	}
}

void ABioGameMode::CheckWinConditions()
{
	bool bAnyStaffAlive = false;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		ABioPlayerState* BioPS = Cast<ABioPlayerState>(PS);
		if (BioPS && BioPS->GameRole == EBioPlayerRole::Staff)
		{
			if (BioPS->Status == EBioPlayerStatus::Alive || BioPS->Status == EBioPlayerStatus::Jailed)
			{
				bAnyStaffAlive = true;
				break;
			}
		}
	}

	if (!bAnyStaffAlive)
	{
		BioGameState->SetGamePhase(EBioGamePhase::End);
	}
}
