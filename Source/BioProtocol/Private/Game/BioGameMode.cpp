// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioGameMode.h"
#include "Game/BioGameState.h"
#include "Game/BioPlayerState.h"
#include "Character/BioPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Character/StaffStatusComponent.h"
#include <Character/ThirdSpectatorPawn.h>
#include <Character/StaffCharacter.h>
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

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

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ABioGameMode::CreateGameVoiceChannels, 2.0f, false);
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

void ABioGameMode::CreateGameVoiceChannels()
{
	TArray<APlayerController*> AllPlayers;
	TArray<APlayerController*> CleanerPlayers;

	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC) continue;

		ABioPlayerState* PS = PC->GetPlayerState<ABioPlayerState>();
		if (!PS || PS->EOSPlayerName.IsEmpty())
		{
			FTimerHandle RetryTimer;
			GetWorldTimerManager().SetTimer(RetryTimer, this, &ABioGameMode::CreateGameVoiceChannels, 1.0f, false);
			return;
		}

		AllPlayers.Add(PC);
		if (PS->GameRole == EBioPlayerRole::Cleaner)
		{
			CleanerPlayers.Add(PC);
		}
	}

	CreatePublicChannel(AllPlayers);

	if (CleanerPlayers.Num() > 0)
	{
		CreateRoleBasedChannel(EBioPlayerRole::Cleaner, CleanerPlayers);
	}
}

void ABioGameMode::CreatePublicChannel(const TArray<APlayerController*>& Players)
{
	FString ChannelName = FString::Printf(TEXT("Game_Public_%s"), *FGuid::NewGuid().ToString());
	RequestCreateChannel(ChannelName, Players, true);
}

void ABioGameMode::CreateRoleBasedChannel(EBioPlayerRole BioRole, const TArray<APlayerController*>& Players)
{
	FString ChannelName = FString::Printf(TEXT("Game_Cleaner_%s"), *FGuid::NewGuid().ToString());
	RequestCreateChannel(ChannelName, Players, false);
}

void ABioGameMode::RequestCreateChannel(const FString& ChannelName, const TArray<APlayerController*>& Players, bool bIs3D)
{
	if (Players.Num() == 0) return;

	ABioPlayerState* FirstPS = Players[0]->GetPlayerState<ABioPlayerState>();

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(TrustedServerUrl + TEXT("/voice/create-channel"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject);
	JsonObj->SetStringField(TEXT("channelName"), ChannelName);
	JsonObj->SetStringField(TEXT("productUserId"), FirstPS->EOSPlayerName);
	JsonObj->SetStringField(TEXT("roomId"), ChannelName);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	Request->SetContentAsString(RequestBody);

	Request->OnProcessRequestComplete().BindLambda(
		[this, Players, ChannelName](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
		{
			if (!bSuccess || !Res.IsValid()) return;

			TSharedPtr<FJsonObject> JsonRes;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonRes) || !JsonRes->GetBoolField(TEXT("success"))) return;

			FString BaseUrl = JsonRes->GetStringField(TEXT("clientBaseUrl"));
			FString FirstToken = JsonRes->GetStringField(TEXT("participantToken"));

			if (ABioPlayerController* PC = Cast<ABioPlayerController>(Players[0]))
			{
				PC->Client_JoinGameChannel(ChannelName, BaseUrl, FirstToken);
			}

			for (int32 i = 1; i < Players.Num(); ++i)
			{

			}
		}
	);

	Request->ProcessRequest();
}

void ABioGameMode::SetPlayerSpectating(AController* VictimController)
{
	if (!VictimController) return;

	APawn* KilledPawn = VictimController->GetPawn();
	if (KilledPawn)
	{
		KilledPawn->Destroy();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = VictimController;

	FVector SpawnLoc = KilledPawn ? KilledPawn->GetActorLocation() : FVector::ZeroVector;
	FRotator SpawnRot = FRotator::ZeroRotator;

	AThirdSpectatorPawn* Spectator = GetWorld()->SpawnActor<AThirdSpectatorPawn>(
		SpectatorPawnClass,
		SpawnLoc,
		SpawnRot,
		SpawnParams
	);

	if (Spectator)
	{
		VictimController->Possess(Spectator);

		VictimController->SetControlRotation(Spectator->GetActorRotation());

		if (APlayerController* PC = Cast<APlayerController>(VictimController))
		{

			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}

		Spectator->SpectateNextPlayer();
	}

}