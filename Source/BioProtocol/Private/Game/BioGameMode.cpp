// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioGameMode.h"
#include "Game/BioGameState.h"
#include "Game/BioPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Character/StaffStatusComponent.h"
#include <Character/ThirdSpectatorPawn.h>
#include <Character/StaffCharacter.h>
#include "Character/BioPlayerController.h"
#include "VoiceChannelManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
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
}


void ABioGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || bGameVoiceStarted)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FString MapName = World->GetMapName();

	// PIE 프리픽스 제거
	if (MapName.StartsWith(TEXT("UEDPIE_")))
	{
		int32 UnderscoreIndex;
		if (MapName.FindChar('_', UnderscoreIndex))
		{
			MapName = MapName.RightChop(UnderscoreIndex + 1);
		}
	}

	if (MapName.Contains(TEXT("/")))
	{
		int32 LastSlashIndex;
		MapName.FindLastChar('/', LastSlashIndex);
		MapName = MapName.RightChop(LastSlashIndex + 1);
	}

	bool bIsGameMap = MapName.Contains(TEXT("TestSession")) || MapName.Contains(TEXT("GameMap"));

	if (bIsGameMap)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Game map loaded, waiting for players..."));

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
			{
				const int32 PlayerCount = GetNumPlayers();

				if (PlayerCount > 0 && !bGameVoiceStarted)
				{
					bGameVoiceStarted = true;
					UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Players loaded (%d), starting game"), PlayerCount);
					StartGame();
				}
				else if (PlayerCount == 0)
				{
					UE_LOG(LogTemp, Error, TEXT("[GameMode] ✗ No players found, returning to lobby"));
					GetWorld()->ServerTravel("/Game/Level/Lobby?listen");
				}
			}, 15.0f, false);
	}
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

void ABioGameMode::StartGame()
{
	if (!HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Game starting..."));

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UVoiceChannelManager* VCM = GI->GetSubsystem<UVoiceChannelManager>())
		{
			VCM->OnGameStart();
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			this->CreateGameVoiceChannels();
		}, 1.0f, false);
}

void ABioGameMode::EndGame()
{
	if (!HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Game ending, returning to lobby..."));

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UVoiceChannelManager* VCM = GI->GetSubsystem<UVoiceChannelManager>())
		{
			VCM->OnGameEnd();
		}
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			GetWorld()->ServerTravel("/Game/Level/Lobby?listen");
		}, 2.0f, false);
}

void ABioGameMode::CreateGameVoiceChannels()
{
	TArray<APlayerController*> AllPlayers;
	TArray<APlayerController*> MafiaPlayers;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (ABioPlayerController* BioPC = Cast<ABioPlayerController>(PC))
			{
				if (ABioPlayerState* PS = BioPC->GetPlayerState<ABioPlayerState>())
				{
					if (PS->EOSPlayerName.IsEmpty())
					{
						FTimerHandle RetryTimer;
						GetWorldTimerManager().SetTimer(RetryTimer, [this]()
							{
								this->CreateGameVoiceChannels();
							}, 0.5f, false);
						return;
					}

					AllPlayers.Add(PC);

					if (PS->GameRole == EBioPlayerRole::Cleaner)
					{
						MafiaPlayers.Add(PC);
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Creating voice channels - Total: %d, Mafia: %d"),
		AllPlayers.Num(), MafiaPlayers.Num());

	if (AllPlayers.Num() > 0)
	{
		CreatePublicGameChannel(AllPlayers);
	}

	if (MafiaPlayers.Num() > 0)
	{
		CreateMafiaGameChannel(MafiaPlayers);
	}
}

void ABioGameMode::CreatePublicGameChannel(const TArray<APlayerController*>& Players)
{
	const FString ChannelName = FString::Printf(TEXT("Game_Public_Citizen_%s"),
		*FGuid::NewGuid().ToString(EGuidFormats::Short));

	CreateGameChannel(EBioPlayerRole::Staff, Players, ChannelName);
}

void ABioGameMode::CreateMafiaGameChannel(const TArray<APlayerController*>& Players)
{
	const FString ChannelName = FString::Printf(TEXT("Game_Private_Mafia_%s"),
		*FGuid::NewGuid().ToString(EGuidFormats::Short));

	CreateGameChannel(EBioPlayerRole::Cleaner, Players, ChannelName);
}


void ABioGameMode::CreateGameChannel(EBioPlayerRole Team, const TArray<APlayerController*>& Players, const FString& ChannelName)
{
	TArray<ABioPlayerState*> ValidPlayerStates;
	TArray<APlayerController*> ValidControllers;

	for (APlayerController* PC : Players)
	{
		ABioPlayerController* BioPC = Cast<ABioPlayerController>(PC);
		if (!BioPC) continue;

		ABioPlayerState* PS = BioPC->GetPlayerState<ABioPlayerState>();
		if (!PS || PS->EOSPlayerName.IsEmpty()) continue;

		if (Team == EBioPlayerRole::Cleaner && PS->GameRole != EBioPlayerRole::Cleaner)
			continue;

		ValidPlayerStates.Add(PS);
		ValidControllers.Add(PC);
	}

	if (ValidPlayerStates.Num() == 0)
	{
		return;
	}

	const FString RoomId = ChannelName;
	ABioPlayerState* FirstPS = ValidPlayerStates[0];

	TSharedRef<IHttpRequest> CreateRequest = FHttpModule::Get().CreateRequest();
	CreateRequest->SetURL(TrustedServerUrl + TEXT("/voice/create-channel"));
	CreateRequest->SetVerb(TEXT("POST"));
	CreateRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> CreateJson = MakeShareable(new FJsonObject);
	CreateJson->SetStringField(TEXT("channelName"), ChannelName);
	CreateJson->SetStringField(TEXT("productUserId"), FirstPS->EOSPlayerName);
	CreateJson->SetStringField(TEXT("roomId"), RoomId);

	FString CreateContent;
	TSharedRef<TJsonWriter<>> CreateWriter = TJsonWriterFactory<>::Create(&CreateContent);
	FJsonSerializer::Serialize(CreateJson.ToSharedRef(), CreateWriter);
	CreateRequest->SetContentAsString(CreateContent);

	CreateRequest->OnProcessRequestComplete().BindLambda(
		[this, Team, ValidControllers, ValidPlayerStates, ChannelName, RoomId]
	(FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
		{
			if (!bSuccess || !Res.IsValid())
			{
				return;
			}

			TSharedPtr<FJsonObject> JsonResponse;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());

			if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse->GetBoolField(TEXT("success")))
			{
				return;
			}

			const FString ClientBaseUrl = JsonResponse->GetStringField(TEXT("clientBaseUrl"));
			const FString FirstToken = JsonResponse->GetStringField(TEXT("participantToken"));

			if (ValidControllers.Num() > 0)
			{
				if (ABioPlayerController* FirstPC = Cast<ABioPlayerController>(ValidControllers[0]))
				{
					FirstPC->Client_JoinGameChannel(ChannelName, ClientBaseUrl, FirstToken);
				}
			}

			for (int32 i = 1; i < ValidPlayerStates.Num(); i++)
			{
				ABioPlayerState* PS = ValidPlayerStates[i];

				TSharedRef<IHttpRequest> AddRequest = FHttpModule::Get().CreateRequest();
				AddRequest->SetURL(TrustedServerUrl + TEXT("/voice/add-participant"));
				AddRequest->SetVerb(TEXT("POST"));
				AddRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

				TSharedPtr<FJsonObject> AddJson = MakeShareable(new FJsonObject);
				AddJson->SetStringField(TEXT("roomId"), RoomId);
				AddJson->SetStringField(TEXT("productUserId"), PS->EOSPlayerName);

				FString AddContent;
				TSharedRef<TJsonWriter<>> AddWriter = TJsonWriterFactory<>::Create(&AddContent);
				FJsonSerializer::Serialize(AddJson.ToSharedRef(), AddWriter);
				AddRequest->SetContentAsString(AddContent);

				const int32 PlayerIndex = i;
				AddRequest->OnProcessRequestComplete().BindLambda(
					[this, ValidControllers, PlayerIndex, ChannelName, ClientBaseUrl]
				(FHttpRequestPtr Req2, FHttpResponsePtr Res2, bool bSuccess2)
					{
						if (!bSuccess2 || !Res2.IsValid())
						{
							return;
						}

						TSharedPtr<FJsonObject> AddResponse;
						TSharedRef<TJsonReader<>> Reader2 = TJsonReaderFactory<>::Create(Res2->GetContentAsString());

						if (!FJsonSerializer::Deserialize(Reader2, AddResponse) || !AddResponse->GetBoolField(TEXT("success")))
						{
							return;
						}

						const FString ParticipantToken = AddResponse->GetStringField(TEXT("participantToken"));

						if (PlayerIndex < ValidControllers.Num())
						{
							if (ABioPlayerController* PC = Cast<ABioPlayerController>(ValidControllers[PlayerIndex]))
							{
								PC->Client_JoinGameChannel(ChannelName, ClientBaseUrl, ParticipantToken);
							}
						}
					}
					);

				AddRequest->ProcessRequest();
			}
		}
		);

	CreateRequest->ProcessRequest();
}




