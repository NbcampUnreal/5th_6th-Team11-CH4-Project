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

void ATestGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
}

void ATestGameMode::StartGame()
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

void ATestGameMode::CreateGameVoiceChannels()
{
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

void ATestGameMode::CreatePublicGameChannel(const TArray<APlayerController*>& Players)
{
    const FString ChannelName = FString::Printf(TEXT("Game_Public_Citizen_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Short));

    CreateGameChannel(EVoiceTeam::Citizen, Players, ChannelName);
}

void ATestGameMode::CreateMafiaGameChannel(const TArray<APlayerController*>& Players)
{
    const FString ChannelName = FString::Printf(TEXT("Game_Private_Mafia_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Short));

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

        if (Team == EVoiceTeam::Mafia && PS->VoiceTeam != EVoiceTeam::Mafia)
            continue;

        ValidPlayerStates.Add(PS);
        ValidControllers.Add(PC);
    }

    if (ValidPlayerStates.Num() == 0)
    {
        return;
    }

    const FString RoomId = ChannelName;
    ATESTPlayerState* FirstPS = ValidPlayerStates[0];

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
                if (ATestController* FirstPC = Cast<ATestController>(ValidControllers[0]))
                {
                    FirstPC->Client_JoinGameChannel(ChannelName, ClientBaseUrl, FirstToken);
                }
            }

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
                            if (ATestController* PC = Cast<ATestController>(ValidControllers[PlayerIndex]))
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

void ATestGameMode::EndGame()
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