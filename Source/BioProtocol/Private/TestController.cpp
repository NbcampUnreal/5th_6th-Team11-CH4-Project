#include "TestController.h"
#include "VoiceChat.h"
#include "../Session/SessionSubsystem.h"
#include "TESTPlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "IOnlineSubsystemEOS.h"
#include "EOSVoiceChat.h"
#include "EOSVoiceChatTypes.h"

void ATestController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        StartProximityVoice();
    }

    // 게임 맵에서는 송신 중지 상태로 시작
    UWorld* World = GetWorld();
    if (World)
    {
        FString MapName = World->GetMapName();
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

        if (MapName.Contains(TEXT("TestSession")) || MapName.Contains(TEXT("GameMap")))
        {
            VoiceTransmitToNone();
        }
    }
}

void ATestController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ProximityTimer);
    Super::EndPlay(EndPlayReason);
}

void ATestController::LoginToEOS(int32 Credential)
{
    if (!IsLocalController())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    IOnlineSubsystem* OSS = Online::GetSubsystem(World);
    if (!OSS)
    {
        return;
    }

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return;
    }

    const int32 LocalUserNum = 0;
    if (Identity->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn || bLoginInProgress)
    {
        return;
    }

    FString DevAuthAddr = TEXT("localhost:8081");
    FParse::Value(FCommandLine::Get(), TEXT("EOSDevAuth="), DevAuthAddr);

    FString DevToken = FString::Printf(TEXT("DevUser%d"), FMath::Max(1, Credential));

    if (LoginCompleteHandle.IsValid())
    {
        Identity->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteHandle);
        LoginCompleteHandle.Reset();
    }

    bLoginInProgress = true;

    FOnlineAccountCredentials Creds;
    Creds.Type = TEXT("developer");
    Creds.Id = DevAuthAddr;
    Creds.Token = DevToken;

    LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(LocalUserNum,
        FOnLoginCompleteDelegate::CreateUObject(this, &ATestController::HandleLoginComplete));

    Identity->Login(LocalUserNum, Creds);
}

void ATestController::CreateLobby(const FString& Ip, int32 Port, int32 PublicConnections)
{
    if (!IsLocalController())
    {
        return;
    }

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    USessionSubsystem* SS = GI->GetSubsystem<USessionSubsystem>();
    if (!SS) return;

    SS->CreateLobbyForDedicated(Ip, Port, PublicConnections);
}

void ATestController::HandleLoginComplete(int32, bool bOk, const FUniqueNetId& Id, const FString& Err)
{
    if (UWorld* World = GetWorld())
    {
        if (IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World))
        {
            if (LoginCompleteHandle.IsValid())
            {
                Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
                LoginCompleteHandle.Reset();
            }
        }
    }

    bLoginInProgress = false;

    if (!bOk)
    {
        UE_LOG(LogTemp, Error, TEXT("[EOS] ✗ Login failed: %s"), *Err);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[EOS] ✓ Login successful: %s"), *Id.ToString());

    Server_SetEOSPlayerName(Id.ToString());
    CacheVoiceChatUser();
}

void ATestController::Server_StartGameSession_Implementation()
{
    if (!HasAuthority())
        return;

    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    USessionSubsystem* SessionSub = GI->GetSubsystem<USessionSubsystem>();
    if (SessionSub)
    {
        SessionSub->StartGameSession();
    }
}

void ATestController::Server_SetReady_Implementation()
{
    if (!HasAuthority())
    {
        return;
    }

    ATESTPlayerState* PS = GetPlayerState<ATESTPlayerState>();
    if (PS)
    {
        PS->bisReady = !PS->bisReady;
    }
}

void ATestController::StartProximityVoice()
{
    GetWorldTimerManager().SetTimer(ProximityTimer, this, &ATestController::UpdateProximityVoice, ProxUpdateInterval, true);
}

void ATestController::UpdateProximityVoice()
{
    if (!IsLocalController())
        return;

    CacheVoiceChatUser();
    if (!VoiceChatUser || PublicGameChannelName.IsEmpty())
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    FVector ListenerLoc;
    FVector ListenerForward = FVector::ForwardVector;
    FVector ListenerUp = FVector::UpVector;

    if (APawn* MyPawn = GetPawn())
    {
        ListenerLoc = MyPawn->GetActorLocation();
        ListenerForward = MyPawn->GetActorForwardVector();
        ListenerUp = MyPawn->GetActorUpVector();
    }
    else if (PlayerCameraManager)
    {
        ListenerLoc = PlayerCameraManager->GetCameraLocation();
        ListenerForward = PlayerCameraManager->GetActorForwardVector();
        ListenerUp = PlayerCameraManager->GetActorUpVector();
    }
    else
    {
        return;
    }

    ATESTPlayerState* MyPS = GetPlayerState<ATESTPlayerState>();
    const bool bIAmMafia = (MyPS && MyPS->VoiceTeam == EVoiceTeam::Mafia);

    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* OtherPawn = *It;
        if (!OtherPawn || OtherPawn == GetPawn())
            continue;

        ATESTPlayerState* OtherPS = Cast<ATESTPlayerState>(OtherPawn->GetPlayerState());
        if (!OtherPS || OtherPS->EOSPlayerName.IsEmpty())
            continue;

        const bool bOtherIsMafia = (OtherPS->VoiceTeam == EVoiceTeam::Mafia);

        FVector SpeakerLoc = OtherPawn->GetActorLocation();
        float Distance = FVector::Dist(ListenerLoc, SpeakerLoc);

        float Volume = CalcProxVolume01(Distance, ProxMinDist, ProxMaxDist);

        VoiceChatUser->Set3DPosition(PublicGameChannelName, SpeakerLoc, ListenerLoc, ListenerForward, ListenerUp);

        // 마피아끼리는 항상 최대 볼륨
        if (bIAmMafia && bOtherIsMafia)
        {
            Volume = 1.0f;
        }

        VoiceChatUser->SetPlayerVolume(OtherPS->EOSPlayerName, Volume);
    }
}

float ATestController::CalcProxVolume01(float Dist, float MinD, float MaxD)
{
    if (Dist <= MinD) return 1.0f;
    if (Dist >= MaxD) return 0.0f;

    const float Alpha = (Dist - MinD) / (MaxD - MinD);
    return FMath::Clamp(1.0f - (Alpha * Alpha), 0.0f, 1.0f);
}

void ATestController::Server_SetEOSPlayerName_Implementation(const FString& InEOSPlayerName)
{
    if (!HasAuthority())
        return;

    ATESTPlayerState* PS = GetPlayerState<ATESTPlayerState>();
    if (PS)
    {
        PS->Server_SetEOSPlayerName(InEOSPlayerName);
    }
}

void ATestController::CacheVoiceChatUser()
{
    if (VoiceChatUser)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    IOnlineSubsystem* OSS = Online::GetSubsystem(World, TEXT("EOS"));
    if (!OSS)
        return;

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
        return;

    TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
    if (!LocalUserId.IsValid())
        return;

    IOnlineSubsystemEOS* EOS = static_cast<IOnlineSubsystemEOS*>(OSS);
    if (EOS)
    {
        VoiceChatUser = EOS->GetVoiceChatUserInterface(*LocalUserId);
    }
}

void ATestController::VoiceTransmitToNone()
{
    CacheVoiceChatUser();
    if (VoiceChatUser)
    {
        VoiceChatUser->TransmitToNoChannels();
    }
}

void ATestController::VoiceTransmitToPublic()
{
    CacheVoiceChatUser();
    if (!VoiceChatUser)
        return;

    FString ChannelToUse = PublicGameChannelName.IsEmpty() ? LobbyChannelName : PublicGameChannelName;
    if (ChannelToUse.IsEmpty())
    {
        return;
    }

    TSet<FString> Channels = { ChannelToUse };
    VoiceChatUser->TransmitToSpecificChannels(Channels);
}

void ATestController::VoiceTransmitToMafiaOnly()
{
    CacheVoiceChatUser();
    if (!VoiceChatUser || MafiaGameChannelName.IsEmpty())
        return;

    TSet<FString> Channels = { MafiaGameChannelName };
    VoiceChatUser->TransmitToSpecificChannels(Channels);
}

void ATestController::VoiceTransmitToBothChannels()
{
    CacheVoiceChatUser();
    if (!VoiceChatUser || PublicGameChannelName.IsEmpty() || MafiaGameChannelName.IsEmpty())
        return;

    TSet<FString> Channels = { PublicGameChannelName, MafiaGameChannelName };
    VoiceChatUser->TransmitToSpecificChannels(Channels);
}

void ATestController::Client_JoinLobbyChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
    JoinLobbyChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ATestController::JoinLobbyChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
    if (!IsLocalController())
        return;

    CacheVoiceChatUser();
    if (!VoiceChatUser)
        return;

    LobbyChannelName = ChannelName;

    FEOSVoiceChatChannelCredentials Creds;
    Creds.ClientBaseUrl = ClientBaseUrl;
    Creds.ParticipantToken = ParticipantToken;

    VoiceChatUser->JoinChannel(ChannelName, Creds.ToJson(), EVoiceChatChannelType::NonPositional,
        FOnVoiceChatChannelJoinCompleteDelegate::CreateLambda(
            [this](const FString& JoinedChannel, const FVoiceChatResult& Result)
            {
                if (Result.IsSuccess())
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Joined lobby channel"));
                    VoiceChatUser->TransmitToAllChannels();
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join lobby: %s"), *Result.ErrorDesc);
                }
            }
        )
    );
}

void ATestController::LeaveLobbyChannel()
{
    if (!IsLocalController())
        return;

    CacheVoiceChatUser();
    if (!VoiceChatUser || LobbyChannelName.IsEmpty())
        return;

    VoiceChatUser->LeaveChannel(LobbyChannelName,
        FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
            [this](const FString& LeftChannel, const FVoiceChatResult& Result)
            {
                if (Result.IsSuccess())
                {
                    UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left lobby channel"));
                    LobbyChannelName.Empty();
                }
            }
        )
    );
}

void ATestController::Client_JoinGameChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
    JoinGameChannel_Local(ChannelName, ClientBaseUrl, ParticipantToken);
}

void ATestController::JoinGameChannel_Local(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
    if (!IsLocalController())
        return;

    CacheVoiceChatUser();
    if (!VoiceChatUser)
        return;

    bool bIsMafiaChannel = ChannelName.Contains(TEXT("Mafia"));
    bool bIsPublicChannel = ChannelName.Contains(TEXT("Citizen")) || ChannelName.Contains(TEXT("Public"));

    EVoiceChatChannelType ChannelType = EVoiceChatChannelType::NonPositional;

    if (bIsMafiaChannel)
    {
        MafiaGameChannelName = ChannelName;
        ChannelType = EVoiceChatChannelType::NonPositional;
    }
    else if (bIsPublicChannel)
    {
        PublicGameChannelName = ChannelName;
        ChannelType = EVoiceChatChannelType::Positional;
    }
    else
    {
        return;
    }

    FEOSVoiceChatChannelCredentials Creds;
    Creds.ClientBaseUrl = ClientBaseUrl;
    Creds.ParticipantToken = ParticipantToken;

    VoiceChatUser->JoinChannel(ChannelName, Creds.ToJson(), ChannelType,
        FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(this, &ATestController::OnVoiceChannelJoined)
    );
}

void ATestController::LeaveGameChannels()
{
    if (!IsLocalController())
        return;

    CacheVoiceChatUser();
    if (!VoiceChatUser)
        return;

    if (!PublicGameChannelName.IsEmpty())
    {
        VoiceChatUser->LeaveChannel(PublicGameChannelName,
            FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
                [this](const FString&, const FVoiceChatResult& Result)
                {
                    if (Result.IsSuccess())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left public game channel"));
                        PublicGameChannelName.Empty();
                    }
                }
            )
        );
    }

    if (!MafiaGameChannelName.IsEmpty())
    {
        VoiceChatUser->LeaveChannel(MafiaGameChannelName,
            FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda(
                [this](const FString&, const FVoiceChatResult& Result)
                {
                    if (Result.IsSuccess())
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Left mafia game channel"));
                        MafiaGameChannelName.Empty();
                    }
                }
            )
        );
    }
}

void ATestController::OnVoiceChannelJoined(const FString& ChannelName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Voice] ✓ Joined game channel: %s"), *ChannelName);
        VoiceTransmitToPublic();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[Voice] ✗ Failed to join game channel: %s"), *Result.ErrorDesc);
    }
}