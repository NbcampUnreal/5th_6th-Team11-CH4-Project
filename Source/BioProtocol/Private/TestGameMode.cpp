#include "TestGameMode.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "TestController.h"
#include "TESTPlayerState.h"

void ATestGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UE_LOG(LogTemp, Log, TEXT("[GameMode] Player logged in. Total players: %d"), GetNumPlayers());

    // 예시: 2명 이상일 때 테스트 채널 생성
    if (GetNumPlayers() >= 2)
    {
        TArray<APlayerController*> AllPlayers;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (APlayerController* PC = It->Get())
            {
                AllPlayers.Add(PC);
            }
        }

        // 테스트용으로 Citizen 팀 채널 생성
        CreatePrivateVoiceChannel(EVoiceTeam::Citizen, AllPlayers);
    }
}

void ATestGameMode::Logout(AController* Exiting)
{
    UE_LOG(LogTemp, Log, TEXT("[GameMode] Player logged out"));
    Super::Logout(Exiting);
}

void ATestGameMode::CreatePrivateVoiceChannel(EVoiceTeam Team, const TArray<APlayerController*>& Players)
{
    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Error, TEXT("[Voice] CreatePrivateVoiceChannel must be called on server"));
        return;
    }

    if (Players.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Voice] No players to add to channel"));
        return;
    }

    // 유효한 플레이어만 필터링
    TArray<ATESTPlayerState*> ValidPlayerStates;
    for (APlayerController* PC : Players)
    {
        ATestController* TestPC = Cast<ATestController>(PC);
        if (!TestPC) continue;

        ATESTPlayerState* PS = TestPC->GetPlayerState<ATESTPlayerState>();
        if (PS && !PS->EOSPlayerName.IsEmpty())
        {
            ValidPlayerStates.Add(PS);
        }
    }

    if (ValidPlayerStates.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[Voice] No valid players with EOS names"));
        return;
    }

    // 채널 이름 생성
    const FString TeamName = (Team == EVoiceTeam::Mafia) ? TEXT("Mafia") : TEXT("Citizen");
    const FString ChannelName = FString::Printf(TEXT("Private_%s_%s"),
        *TeamName,
        *FGuid::NewGuid().ToString(EGuidFormats::Short));
    const FString RoomId = ChannelName;

    UE_LOG(LogTemp, Log, TEXT("[Voice] Creating channel '%s' for %d players (Team: %s)"),
        *ChannelName, ValidPlayerStates.Num(), *TeamName);

    // 1단계: 첫 번째 플레이어로 채널 생성
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

    // 람다에서 사용할 변수들을 캡처
    CreateRequest->OnProcessRequestComplete().BindLambda(
        [this, Team, Players, ValidPlayerStates, ChannelName, RoomId]
    (FHttpRequestPtr Req, FHttpResponsePtr Res, bool bSuccess)
        {
            if (!bSuccess || !Res.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[Voice] Failed to create channel"));
                return;
            }

            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Res->GetContentAsString());

            if (!FJsonSerializer::Deserialize(Reader, JsonResponse) || !JsonResponse->GetBoolField(TEXT("success")))
            {
                UE_LOG(LogTemp, Error, TEXT("[Voice] Channel creation failed: %s"), *Res->GetContentAsString());
                return;
            }

            const FString ClientBaseUrl = JsonResponse->GetStringField(TEXT("clientBaseUrl"));
            const FString FirstToken = JsonResponse->GetStringField(TEXT("participantToken"));

            UE_LOG(LogTemp, Log, TEXT("[Voice] Channel created: %s"), *ChannelName);

            // 첫 번째 플레이어 입장
            if (Players.Num() > 0)
            {
                if (ATestController* FirstPC = Cast<ATestController>(Players[0]))
                {
                    FirstPC->Client_JoinPrivateVoiceChannel(ChannelName, ClientBaseUrl, FirstToken);
                    UE_LOG(LogTemp, Log, TEXT("[Voice] Sent join command to Player 0"));
                }
            }

            // 2단계: 나머지 플레이어들 추가
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

                // 각 플레이어별 토큰을 받아 입장 명령
                const int32 PlayerIndex = i; // 캡처용
                AddRequest->OnProcessRequestComplete().BindLambda(
                    [this, Players, PlayerIndex, ChannelName, ClientBaseUrl]
                (FHttpRequestPtr Req2, FHttpResponsePtr Res2, bool bSuccess2)
                    {
                        if (!bSuccess2 || !Res2.IsValid())
                        {
                            UE_LOG(LogTemp, Error, TEXT("[Voice] Failed to add player %d"), PlayerIndex);
                            return;
                        }

                        TSharedPtr<FJsonObject> AddResponse;
                        TSharedRef<TJsonReader<>> Reader2 = TJsonReaderFactory<>::Create(Res2->GetContentAsString());

                        if (!FJsonSerializer::Deserialize(Reader2, AddResponse) || !AddResponse->GetBoolField(TEXT("success")))
                        {
                            UE_LOG(LogTemp, Error, TEXT("[Voice] Add player %d failed: %s"),
                                PlayerIndex, *Res2->GetContentAsString());
                            return;
                        }

                        const FString ParticipantToken = AddResponse->GetStringField(TEXT("participantToken"));

                        if (PlayerIndex < Players.Num())
                        {
                            if (ATestController* PC = Cast<ATestController>(Players[PlayerIndex]))
                            {
                                PC->Client_JoinPrivateVoiceChannel(ChannelName, ClientBaseUrl, ParticipantToken);
                                UE_LOG(LogTemp, Log, TEXT("[Voice] Sent join command to Player %d"), PlayerIndex);
                            }
                        }
                    }
                    );

                AddRequest->ProcessRequest();
                UE_LOG(LogTemp, Log, TEXT("[Voice] Adding player %d (%s) to channel"), i, *PS->EOSPlayerName);
            }
        }
        );

    CreateRequest->ProcessRequest();
}