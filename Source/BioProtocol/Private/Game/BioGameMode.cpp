// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/BioGameMode.h"
#include "Game/BioGameState.h"
#include "Game/BioPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
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

void ABioGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
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
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
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




