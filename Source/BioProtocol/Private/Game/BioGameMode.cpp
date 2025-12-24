// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioGameMode.h"
#include "Game/BioGameState.h"
#include "Game/BioPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Character/StaffStatusComponent.h"

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
			UE_LOG(LogTemp, Log, TEXT("-> Player [%s] assigned as CLEANER"), *BioPS->GetPlayerName());
		}
		else
		{
			BioPS->GameRole = EBioPlayerRole::Staff;
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
		APawn* Pawn = PS->GetPawn();
		if (!Pawn) continue;

		UStaffStatusComponent* StatusComp = Pawn->FindComponentByClass<UStaffStatusComponent>();
		if (StatusComp && StatusComp->PlayerStatus == EBioPlayerStatus::Jailed)
		{

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
	bool bAnyStaffSurviving = false;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		ABioPlayerState* BioPS = Cast<ABioPlayerState>(PS);
		if (!BioPS) continue;

		if (BioPS->GameRole == EBioPlayerRole::Staff)
		{
			APawn* Pawn = PS->GetPawn();
			UStaffStatusComponent* StatusComp = Pawn ? Pawn->FindComponentByClass<UStaffStatusComponent>() : nullptr;

			if (StatusComp)
			{
				if (StatusComp->PlayerStatus == EBioPlayerStatus::Alive ||
					StatusComp->PlayerStatus == EBioPlayerStatus::Jailed)
				{
					bAnyStaffSurviving = true;
					break;
				}
			}
		}
	}

	if (!bAnyStaffSurviving)
	{
		UE_LOG(LogTemp, Error, TEXT("!!! CLEANER WINS !!! All Staff Incinerated."));

		if (ABioGameState* BioGS = GetGameState<ABioGameState>())
		{
			BioGS->SetGamePhase(EBioGamePhase::End);
		}
	}
}
