#include "VoiceChannelManager.h"
#include "Character/BioPlayerController.h"
#include "Engine/World.h"

void UVoiceChannelManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UVoiceChannelManager::HandlePostLoadMap);
}

void UVoiceChannelManager::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

    Super::Deinitialize();
}

void UVoiceChannelManager::RegisterLobbyChannel(const FString& ChannelName)
{
    LobbyChannelName = ChannelName;
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Lobby channel registered"));
}

void UVoiceChannelManager::RegisterLobbyChannelCredentials(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
    LobbyChannelName = ChannelName;
    LobbyClientBaseUrl = ClientBaseUrl;
    LobbyParticipantToken = ParticipantToken;

    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Lobby channel creds cached"));
}

void UVoiceChannelManager::OnGameStart()
{
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Phase: LOBBY → IN-GAME"));
    CurrentPhase = EGamePhase::InGame;

    if (UWorld* World = GetWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            if (ABioPlayerController* PC = Cast<ABioPlayerController>(It->Get()))
            {
                if (PC->IsLocalController())
                {
                    PC->LeaveLobbyChannel();
                }
            }
        }
    }
}

void UVoiceChannelManager::OnGameEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Phase: IN-GAME → RETURNING"));
    CurrentPhase = EGamePhase::Returning;

    UWorld* World = GetWorld();
    if (!World)
        return;

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (ABioPlayerController* PC = Cast<ABioPlayerController>(It->Get()))
        {
            if (PC->IsLocalController())
            {
                PC->LeaveGameChannels();
            }
        }
    }
}

void UVoiceChannelManager::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!LoadedWorld)
        return;

    // 맵 이름은 PIE에서 UEDPIE_0_Lobby 같은 형태일 수 있으니 Contains로 처리
    const FString MapName = LoadedWorld->GetMapName();
    const bool bIsLobbyMap = MapName.Contains(TEXT("Lobby"));

    if (!bIsLobbyMap)
        return;

    // 게임 종료 후 로비로 복귀 중인 경우에만 로비 채널 재조인
    if (CurrentPhase != EGamePhase::Returning)
        return;

    CurrentPhase = EGamePhase::Lobby;
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Phase: RETURNING → LOBBY (PostLoadMap)"));

    if (LobbyChannelName.IsEmpty() || LobbyClientBaseUrl.IsEmpty() || LobbyParticipantToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] Lobby creds missing, cannot rejoin lobby voice"));
        return;
    }

    for (FConstPlayerControllerIterator It = LoadedWorld->GetPlayerControllerIterator(); It; ++It)
    {
        if (ABioPlayerController* PC = Cast<ABioPlayerController>(It->Get()))
        {
            if (PC->IsLocalController())
            {
                PC->JoinLobbyChannelFromManager(LobbyChannelName, LobbyClientBaseUrl, LobbyParticipantToken);
            }
        }
    }
}
