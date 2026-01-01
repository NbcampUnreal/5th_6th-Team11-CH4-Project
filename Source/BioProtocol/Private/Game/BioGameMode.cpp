// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioGameMode.h"
#include "Game/BioGameState.h"
#include "Game/BioPlayerState.h"
#include "MyTestGameInstance.h"
#include "Game/IsolationDoor.h"
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
#include "EngineUtils.h"



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

	//AssignRoles();

	if (BioGameState)
	{
		BioGameState->SetGamePhase(EBioGamePhase::Day);
		BioGameState->PhaseTimeRemaining = DayDuration;
	}
}

void ABioGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority() || bGameVoiceStarted)
	{
		return;
	}

	// SpawnPoint 세팅
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("SpawnPoint"), Found);

	SpawnPointArray.Reset();
	for (AActor* A : Found)
	{
		SpawnPointArray.Add(A);
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
	bool bIsGameMap = MapName.Contains(TEXT("MainLevel")) || MapName.Contains(TEXT("GameMap"));
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


					// 로딩 화면이 표시될 시간을 주고 게임 시작
					FTimerHandle GameStartTimer;
					GetWorldTimerManager().SetTimer(GameStartTimer, [this]()
						{
							HideLoadingScreenFromAllPlayers();
							AssignRoles();
							StartGame();

						}, 0.5f, false); // 0.5초 후 게임 시작 (필요에 따라 조정)
				}
				else if (PlayerCount == 0)
				{
					UE_LOG(LogTemp, Error, TEXT("[GameMode] ✗ No players found, returning to lobby"));
					GetWorld()->ServerTravel("/Game/Level/Lobby");
				}
			}, 10.0f, false);
	}
}

UClass* ABioGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (!InController) return Super::GetDefaultPawnClassForController_Implementation(InController);

	ABioPlayerState* PS = InController->GetPlayerState<ABioPlayerState>();
	if (!PS) return Super::GetDefaultPawnClassForController_Implementation(InController);

	if (PS->GameRole == EBioPlayerRole::Cleaner)
	{
		if (CleanerPawnClass) return CleanerPawnClass;
	}
	else
	{
		if (StaffPawnClass) return StaffPawnClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void ABioGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 로비 맵인 경우에만 실행
	UWorld* World = GetWorld();
	if (!World) return;

	FString MapName = World->GetMapName();
	if (MapName.StartsWith(TEXT("UEDPIE_")))
	{
		int32 UnderscoreIndex;
		if (MapName.FindChar('_', UnderscoreIndex))
		{
			MapName = MapName.RightChop(UnderscoreIndex + 1);
		}
	}

	bool bIsLobby = MapName.Contains(TEXT("Lobby"));

	if (bIsLobby)
	{
		// 로비 채널이 이미 생성되어 있으면 새 플레이어를 추가
		if (bLobbyVoiceStarted)
		{
			ABioPlayerController* BioPC = Cast<ABioPlayerController>(NewPlayer);
			if (BioPC)
			{
				FTimerHandle TimerHandle;
				GetWorldTimerManager().SetTimer(TimerHandle, [this, BioPC]()
					{
						ABioPlayerState* PS = BioPC->GetPlayerState<ABioPlayerState>();
						if (PS && !PS->EOSPlayerName.IsEmpty())
						{
							const FString ChannelName = FString::Printf(TEXT("Lobby_Voice_%s"),
								*FGuid::NewGuid().ToString(EGuidFormats::Short));
							UE_LOG(LogTemp, Log, TEXT("AddPlayerToExistingChannel Start"));
							AddPlayerToExistingChannel(BioPC, PS, LobbyChannelName);
						}
					}, 2.0f, false);
			}
		}
		else if (!bLobbyVoiceStarted)
		{
			// 첫 플레이어들 - 로비 채널 생성
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, [this]()
				{
					TArray<APlayerController*> AllPlayers;
					for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
					{
						if (APlayerController* PC = It->Get())
						{
							ABioPlayerController* BioPC = Cast<ABioPlayerController>(PC);
							if (BioPC)
							{
								ABioPlayerState* PS = BioPC->GetPlayerState<ABioPlayerState>();
								if (PS && !PS->EOSPlayerName.IsEmpty())
								{
									AllPlayers.Add(PC);
								}
							}
						}
					}

					if (AllPlayers.Num() > 0 && !bLobbyVoiceStarted)
					{
						bLobbyVoiceStarted = true;
						UE_LOG(LogTemp, Log, TEXT("CreateLobbyVoiceChannel Start"));
						CreateLobbyVoiceChannel(AllPlayers);
					}
				}, 2.0f, false);
		}
	}
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

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Total Players: %d, Cleaner Count: %d"), PlayerCount, TargetCleanerNum);

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
			UE_LOG(LogTemp, Log, TEXT("[GameMode] -> Player [%s] assigned as CLEANER"), *BioPS->GetPlayerName());
		}
		else
		{
			BioPS->GameRole = EBioPlayerRole::Staff;
			UE_LOG(LogTemp, Log, TEXT("[GameMode] -> Player [%s] assigned as STAFF"), *BioPS->GetPlayerName());
		}


		BioPS->GameRole = (i < TargetCleanerNum) ? EBioPlayerRole::Cleaner : EBioPlayerRole::Staff;
		BioPS->ForceNetUpdate();

		// 여기서 실제 Pawn 갈아끼우기
		if (AController* OwnerController = Cast<AController>(BioPS->GetOwner()))
		{
			if (APawn* OldPawn = OwnerController->GetPawn())
			{
				OwnerController->UnPossess();
				OldPawn->Destroy();
			}

			UE_LOG(LogTemp, Log, TEXT("[GameMode] RestartPlayer"));

			if (SpawnPointArray.Num() > 0)
			{
				const int32 Index = FMath::RandHelper(SpawnPointArray.Num());
				if (AActor* StartSpot = SpawnPointArray[Index])
				{
					RestartPlayerAtPlayerStart(OwnerController, StartSpot);

					SpawnPointArray.RemoveAtSwap(Index);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("SpawnPointArray is empty"));
				RestartPlayer(OwnerController);
			}
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

void ABioGameMode::SendPlayerToJail(APawn* Victim)
{
	if (!Victim) return;

	if (JailTimers.Contains(Victim)) return;

	AIsolationDoor* SelectedDoor = nullptr;

	for (TActorIterator<AIsolationDoor> It(GetWorld()); It; ++It)
	{
		AIsolationDoor* Door = *It;
		if (Door && !Door->IsOccupied())
		{
			SelectedDoor = Door;
			break;
		}
	}

	if (SelectedDoor)
	{
		FVector SpawnLocation = SelectedDoor->GetJailSpawnLocation();

		Victim->SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);

		SelectedDoor->SetDoorState(false, Victim);

		JailTimers.Add(Victim, MaxJailTime);
		PlayerJailMap.Add(Victim, SelectedDoor);

		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player Jailed at Location: %s"), *SpawnLocation.ToString());
	}
}

void ABioGameMode::UpdateJailTimers(float DeltaTime)
{
	if (JailTimers.Num() == 0) return;

	for (auto It = JailTimers.CreateIterator(); It; ++It)
	{
		APawn* Victim = It.Key();
		float& TimeRemaining = It.Value();

		TimeRemaining -= DeltaTime;

		if (Victim)
		{
			if (UStaffStatusComponent* StatusComp = Victim->FindComponentByClass<UStaffStatusComponent>())
			{
				StatusComp->UpdateJailTime(TimeRemaining);
			}
		}
		if (TimeRemaining <= 0.f)
		{
			ExecutePlayer(Victim);
			It.RemoveCurrent();
		}
	}
}

void ABioGameMode::ReleasePlayerFromJail(APawn* Victim)
{
	if (!Victim || !JailTimers.Contains(Victim)) return;

	JailTimers.Remove(Victim);

	if (UStaffStatusComponent* StatusComp = Victim->FindComponentByClass<UStaffStatusComponent>())
	{
		StatusComp->SetRevived();
	}

	if (PlayerJailMap.Contains(Victim))
	{
		AIsolationDoor* Door = PlayerJailMap[Victim];
		if (Door)
		{
			Door->SetDoorState(true, nullptr);
		}
		PlayerJailMap.Remove(Victim);
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player Rescued!"));
}

void ABioGameMode::ExecutePlayer(APawn* Victim)
{
	if (!Victim) return;

	UE_LOG(LogTemp, Error, TEXT("[GameMode] Time Over! Incinerating Player."));

	if (PlayerJailMap.Contains(Victim))
	{
		AIsolationDoor* Door = PlayerJailMap[Victim];
		if (Door)
		{
			Door->SetDoorState(true, nullptr);
		}
		PlayerJailMap.Remove(Victim);
	}

	Victim->Destroy();
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

		if (UGameInstance* GI = GetGameInstance())
		{
			if (UMyTestGameInstance* MyGI = Cast<UMyTestGameInstance>(GI))
			{
				MyGI->bIsStaffWin = true;
			}
		}

		if (ABioGameState* BioGS = GetGameState<ABioGameState>())
		{
			BioGS->SetGamePhase(EBioGamePhase::End);
			// 테스트 후 EndGame 위치 옮기기
			EndGame();
		}
	}
}

void ABioGameMode::CheckStaffWinConditions()
{
	// 직원이 이기는 경우 -> 탈출 포드 이용 시
	// 탈출 포드 사용 시에 이 함수를 호출
	if (BioGameState->bCanEscape)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UMyTestGameInstance* MyGI = Cast<UMyTestGameInstance>(GI))
			{
				MyGI->bIsStaffWin = true;
			}
		}
	
		UE_LOG(LogTemp, Error, TEXT("!!! STAFF WINS !!! Staff Escaped."));

		if (ABioGameState* BioGS = GetGameState<ABioGameState>())
		{
			BioGS->SetGamePhase(EBioGamePhase::End);
			// 테스트 후 EndGame 위치 옮기기
			EndGame();
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

	// 모든 플레이어가 로비 채널을 떠나도록 명령
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ABioPlayerController* BioPC = Cast<ABioPlayerController>(It->Get()))
		{
			BioPC->LeaveLobbyChannel();
		}
	}

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
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				if (ABioPlayerController* BioPC = Cast<ABioPlayerController>(It->Get()))
				{
					BioPC->Client_LeaveGameChannels();
				}
			}

			VCM->OnGameEnd();

			if (UMyTestGameInstance* MyGI = Cast<UMyTestGameInstance>(GI))
			{
				MyGI->bIsEndGame = true;
			}
		}
	
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			GetWorld()->ServerTravel("/Game/Level/Lobby");
		}, 2.0f, false);
}

void ABioGameMode::HideLoadingScreenFromAllPlayers()
{
	UE_LOG(LogTemp, Error, TEXT("[GameMode] Start HideLoadingScreenFromAllPlayers"));
	for(FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] HideLoadingScreenFromAllPlayers for"));
		APlayerController* PC = It->Get();
		if (PC)
		{
			UE_LOG(LogTemp, Error, TEXT("[GameMode] HideLoadingScreenFromAllPlayers Get Controller"));
			if (ABioPlayerController* BioPC = Cast<ABioPlayerController>(PC))
			{
				UE_LOG(LogTemp, Error, TEXT("[GameMode] HideLoadingScreenFromAllPlayers Cast Controller"));
				BioPC->ClientHideLoadingScreen();
			}
		}
	}
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

void ABioGameMode::CreateLobbyVoiceChannel(const TArray<APlayerController*>& Players)
{
	FString ChannelName = FString::Printf(TEXT("Lobby_Voice_%s"),
		*FGuid::NewGuid().ToString(EGuidFormats::Short));

	LobbyChannelName = ChannelName;

	TArray<ABioPlayerState*> ValidPlayerStates;
	TArray<APlayerController*> ValidControllers;

	for (APlayerController* PC : Players)
	{
		ABioPlayerController* BioPC = Cast<ABioPlayerController>(PC);
		if (!BioPC) continue;

		ABioPlayerState* PS = BioPC->GetPlayerState<ABioPlayerState>();
		if (!PS || PS->EOSPlayerName.IsEmpty()) continue;

		ValidPlayerStates.Add(PS);
		ValidControllers.Add(PC);
	}

	if (ValidPlayerStates.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] ✗ No valid players for lobby voice"));
		return;
	}

	const FString RoomId = ChannelName;
	ABioPlayerState* FirstPS = ValidPlayerStates[0];

	// 첫 번째 플레이어로 채널 생성
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
		[this, ValidControllers, ValidPlayerStates, ChannelName, RoomId]
	(FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
		{
			if (!bSuccess || !Res.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("[GameMode] ✗ Failed to create lobby channel"));
				return;
			}

			TSharedPtr<FJsonObject> JsonResponse;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());

			if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse->GetBoolField(TEXT("success")))
			{
				UE_LOG(LogTemp, Error, TEXT("[GameMode] ✗ Lobby channel creation failed"));
				return;
			}

			const FString ClientBaseUrl = JsonResponse->GetStringField(TEXT("clientBaseUrl"));
			const FString FirstToken = JsonResponse->GetStringField(TEXT("participantToken"));

			UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Lobby voice channel created: %s"), *ChannelName);

			// 첫 번째 플레이어 참가
			if (ValidControllers.Num() > 0)
			{
				if (ABioPlayerController* FirstPC = Cast<ABioPlayerController>(ValidControllers[0]))
				{
					FirstPC->Client_JoinLobbyChannel(ChannelName, ClientBaseUrl, FirstToken);
				}
			}

			// 나머지 플레이어들 추가
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
						if (!bSuccess2 || !Res2.IsValid()) return;

						TSharedPtr<FJsonObject> AddResponse;
						TSharedRef<TJsonReader<>> Reader2 = TJsonReaderFactory<>::Create(Res2->GetContentAsString());

						if (!FJsonSerializer::Deserialize(Reader2, AddResponse) || !AddResponse->GetBoolField(TEXT("success")))
							return;

						const FString ParticipantToken = AddResponse->GetStringField(TEXT("participantToken"));

						if (PlayerIndex < ValidControllers.Num())
						{
							if (ABioPlayerController* PC = Cast<ABioPlayerController>(ValidControllers[PlayerIndex]))
							{
								PC->Client_JoinLobbyChannel(ChannelName, ClientBaseUrl, ParticipantToken);
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



void ABioGameMode::AddPlayerToExistingChannel(ABioPlayerController* BioPC, ABioPlayerState* PS, const FString& ChannelName)
{
	TSharedRef<IHttpRequest> AddRequest = FHttpModule::Get().CreateRequest();
	AddRequest->SetURL(TrustedServerUrl + TEXT("/voice/add-participant"));
	AddRequest->SetVerb(TEXT("POST"));
	AddRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> AddJson = MakeShareable(new FJsonObject);
	AddJson->SetStringField(TEXT("roomId"), ChannelName);
	AddJson->SetStringField(TEXT("productUserId"), PS->EOSPlayerName);

	FString AddContent;
	TSharedRef<TJsonWriter<>> AddWriter = TJsonWriterFactory<>::Create(&AddContent);
	FJsonSerializer::Serialize(AddJson.ToSharedRef(), AddWriter);
	AddRequest->SetContentAsString(AddContent);

	AddRequest->OnProcessRequestComplete().BindLambda(
		[BioPC, ChannelName](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
		{
			if (!bSuccess || !Res.IsValid()) return;

			TSharedPtr<FJsonObject> Response;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());

			if (!FJsonSerializer::Deserialize(Reader, Response) || !Response->GetBoolField(TEXT("success")))
				return;

			const FString ClientBaseUrl = Response->GetStringField(TEXT("clientBaseUrl"));
			const FString ParticipantToken = Response->GetStringField(TEXT("participantToken"));

			BioPC->Client_JoinLobbyChannel(ChannelName, ClientBaseUrl, ParticipantToken);

			UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Late-joining player added to lobby voice"));
		}
	);

	AddRequest->ProcessRequest();
}




