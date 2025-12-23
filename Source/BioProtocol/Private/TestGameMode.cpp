// TestGameMode.cpp
#include "TestGameMode.h"
#include "VoiceChannelManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "TestController.h"
#include "TESTPlayerState.h"

void ATestGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
        return;

    // ✅ 중복 실행 방지
    if (bGameVoiceStarted)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] BeginPlay already processed, skipping"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode] World is NULL!"));
        return;
    }

    // ✅ 맵 이름 추출 개선
    FString WorldName = World->GetName();
    FString MapName = World->GetMapName();

    // PIE 프리픽스 제거
    FString CleanMapName = MapName;
    if (CleanMapName.StartsWith(TEXT("UEDPIE_")))
    {
        int32 UnderscoreIndex;
        if (CleanMapName.FindChar('_', UnderscoreIndex))
        {
            // UEDPIE_0_ 제거
            CleanMapName = CleanMapName.RightChop(UnderscoreIndex + 1);
        }
    }

    // 경로에서 맵 이름만 추출
    if (CleanMapName.Contains(TEXT("/")))
    {
        int32 LastSlashIndex;
        CleanMapName.FindLastChar('/', LastSlashIndex);
        CleanMapName = CleanMapName.RightChop(LastSlashIndex + 1);
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ========== BeginPlay - Map Loaded =========="));
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] World Name: %s"), *WorldName);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Raw Map Name: %s"), *MapName);
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Clean Map Name: %s"), *CleanMapName);

    // ✅ 게임 맵 검증 개선
    bool bIsGameMap = CleanMapName.Equals(TEXT("TestSession"), ESearchCase::IgnoreCase) ||
        CleanMapName.Contains(TEXT("TestSession")) ||
        CleanMapName.Equals(TEXT("GameMap"), ESearchCase::IgnoreCase);

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Is Game Map: %s"), bIsGameMap ? TEXT("YES") : TEXT("NO"));

    if (bIsGameMap)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ This is GAME map, starting voice channels..."));

        // 플레이어 로드 대기
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [this]()
            {
                const int32 PlayerCount = GetNumPlayers();
                UE_LOG(LogTemp, Warning, TEXT("[GameMode] Timer fired - Players: %d, GameVoiceStarted: %s"),
                    PlayerCount, bGameVoiceStarted ? TEXT("YES") : TEXT("NO"));

                if (PlayerCount > 0 && !bGameVoiceStarted)
                {
                    bGameVoiceStarted = true;
                    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Players loaded, starting game..."));
                    StartGame();
                }
                else if (PlayerCount == 0)
                {
                    UE_LOG(LogTemp, Error, TEXT("[GameMode] ✗ No players found after 2 seconds!"));
                }
            }, 2.0f, false);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] This is LOBBY map (%s), skipping game start"), *CleanMapName);
    }
}

void ATestGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UE_LOG(LogTemp, Log, TEXT("[GameMode] Player logged in. Total players: %d"), GetNumPlayers());

    if (ATestController* TestPC = Cast<ATestController>(NewPlayer))
    {
        if (ATESTPlayerState* PS = TestPC->GetPlayerState<ATESTPlayerState>())
        {
            UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player team: %s"),
                PS->VoiceTeam == EVoiceTeam::Mafia ? TEXT("MAFIA") : TEXT("CITIZEN"));
        }
    }
}

void ATestGameMode::StartGame()
{
    if (!HasAuthority())
        return;

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ========== STARTING GAME =========="));

    // 1. VoiceChannelManager에 게임 시작 알림
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UVoiceChannelManager* VCM = GI->GetSubsystem<UVoiceChannelManager>())
        {
            VCM->OnGameStart();
        }
    }

    // 2. 게임 채널 생성
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            this->CreateGameVoiceChannels();
        }, 1.0f, false);
}

void ATestGameMode::CreateGameVoiceChannels()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameMode] CreateGameVoiceChannels called"));

    TArray<APlayerController*> AllPlayers;
    TArray<APlayerController*> MafiaPlayers;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (ATestController* TestPC = Cast<ATestController>(PC))
            {
                if (ATESTPlayerState* PS = TestPC->GetPlayerState<ATESTPlayerState>())
                {
                    if (PS->EOSPlayerName.IsEmpty())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player has no EOSPlayerName, retrying..."));

                        FTimerHandle RetryTimer;
                        GetWorldTimerManager().SetTimer(RetryTimer, [this]()
                            {
                                this->CreateGameVoiceChannels();
                            }, 0.5f, false);
                        return;
                    }

                    AllPlayers.Add(PC);

                    if (PS->VoiceTeam == EVoiceTeam::Mafia)
                    {
                        MafiaPlayers.Add(PC);
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Total: %d, Mafia: %d, Citizens: %d"),
        AllPlayers.Num(), MafiaPlayers.Num(), AllPlayers.Num() - MafiaPlayers.Num());

    // 공개 게임 채널 생성
    if (AllPlayers.Num() > 0)
    {
        CreatePublicGameChannel(AllPlayers);
    }

    // 마피아 게임 채널 생성
    if (MafiaPlayers.Num() > 0)
    {
        CreateMafiaGameChannel(MafiaPlayers);
    }
}

void ATestGameMode::CreatePublicGameChannel(const TArray<APlayerController*>& Players)
{
    const FString ChannelName = FString::Printf(TEXT("Game_Public_Citizen_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Short));

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Creating PUBLIC channel: %s"), *ChannelName);
    CreateGameChannel(EVoiceTeam::Citizen, Players, ChannelName);
}

void ATestGameMode::CreateMafiaGameChannel(const TArray<APlayerController*>& Players)
{
    const FString ChannelName = FString::Printf(TEXT("Game_Private_Mafia_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Short));

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] Creating MAFIA channel: %s"), *ChannelName);
    CreateGameChannel(EVoiceTeam::Mafia, Players, ChannelName);
}

void ATestGameMode::CreateGameChannel(EVoiceTeam Team, const TArray<APlayerController*>& Players, const FString& ChannelName)
{
    TArray<ATESTPlayerState*> ValidPlayerStates;
    TArray<APlayerController*> ValidControllers;

    for (APlayerController* PC : Players)
    {
        ATestController* TestPC = Cast<ATestController>(PC);
        if (!TestPC) continue;

        ATESTPlayerState* PS = TestPC->GetPlayerState<ATESTPlayerState>();
        if (!PS || PS->EOSPlayerName.IsEmpty()) continue;

        // 마피아 채널은 마피아만
        if (Team == EVoiceTeam::Mafia && PS->VoiceTeam != EVoiceTeam::Mafia)
            continue;

        ValidPlayerStates.Add(PS);
        ValidControllers.Add(PC);
    }

    if (ValidPlayerStates.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode] No valid players for channel: %s"), *ChannelName);
        return;
    }

    const FString RoomId = ChannelName;
    ATESTPlayerState* FirstPS = ValidPlayerStates[0];

    UE_LOG(LogTemp, Log, TEXT("[GameMode] Creating channel '%s' for %d players"), *ChannelName, ValidPlayerStates.Num());

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
                UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to create channel: %s"), *ChannelName);
                return;
            }

            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse->GetBoolField(TEXT("success")))
            {
                UE_LOG(LogTemp, Error, TEXT("[GameMode] Channel creation failed: %s"), *Res->GetContentAsString());
                return;
            }

            const FString ClientBaseUrl = JsonResponse->GetStringField(TEXT("clientBaseUrl"));
            const FString FirstToken = JsonResponse->GetStringField(TEXT("participantToken"));

            UE_LOG(LogTemp, Warning, TEXT("[GameMode] ✓ Game channel created: %s"), *ChannelName);

            // 첫 번째 플레이어 입장
            if (ValidControllers.Num() > 0)
            {
                if (ATestController* FirstPC = Cast<ATestController>(ValidControllers[0]))
                {
                    FirstPC->Client_JoinGameChannel(ChannelName, ClientBaseUrl, FirstToken);
                    UE_LOG(LogTemp, Log, TEXT("[GameMode] Sent join to Player 0"));
                }
            }

            // 나머지 플레이어 추가
            for (int32 i = 1; i < ValidPlayerStates.Num(); i++)
            {
                ATESTPlayerState* PS = ValidPlayerStates[i];

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
                            UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to add player %d"), PlayerIndex);
                            return;
                        }

                        TSharedPtr<FJsonObject> AddResponse;
                        TSharedRef<TJsonReader<>> Reader2 = TJsonReaderFactory<>::Create(Res2->GetContentAsString());

                        if (!FJsonSerializer::Deserialize(Reader2, AddResponse) || !AddResponse->GetBoolField(TEXT("success")))
                        {
                            UE_LOG(LogTemp, Error, TEXT("[GameMode] Add player %d failed"), PlayerIndex);
                            return;
                        }

                        const FString ParticipantToken = AddResponse->GetStringField(TEXT("participantToken"));

                        if (PlayerIndex < ValidControllers.Num())
                        {
                            if (ATestController* PC = Cast<ATestController>(ValidControllers[PlayerIndex]))
                            {
                                PC->Client_JoinGameChannel(ChannelName, ClientBaseUrl, ParticipantToken);
                                UE_LOG(LogTemp, Log, TEXT("[GameMode] Sent join to Player %d"), PlayerIndex);
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

void ATestGameMode::EndGame()
{
    if (!HasAuthority())
        return;

    UE_LOG(LogTemp, Warning, TEXT("[GameMode] ========== ENDING GAME =========="));

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UVoiceChannelManager* VCM = GI->GetSubsystem<UVoiceChannelManager>())
        {
            VCM->OnGameEnd();
        }
    }

    // 로비로 복귀
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            // ✅ 로비 맵 경로 확인
            GetWorld()->ServerTravel("/Game/Level/Lobby");
        }, 2.0f, false);
}