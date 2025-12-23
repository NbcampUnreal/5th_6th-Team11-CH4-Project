// VoiceChannelManager.cpp
#include "VoiceChannelManager.h"
#include "TestController.h"
#include "Engine/World.h"

void UVoiceChannelManager::RegisterLobbyChannel(const FString& ChannelName)
{
    LobbyChannelName = ChannelName;
    UE_LOG(LogTemp, Log, TEXT("[VoiceManager] Lobby channel registered: %s"), *ChannelName);
}

void UVoiceChannelManager::OnGameStart()
{
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ===== GAME STARTING ====="));
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] Phase: LOBBY → IN-GAME"));

    CurrentPhase = EGamePhase::InGame;

    // 모든 로컬 플레이어의 로비 채널 나가기
    if (UWorld* World = GetWorld())
    {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
        {
            if (ATestController* PC = Cast<ATestController>(It->Get()))
            {
                if (PC->IsLocalController())
                {
                    PC->LeaveLobbyChannel();
                    UE_LOG(LogTemp, Log, TEXT("[VoiceManager] Local player left lobby channel"));
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] Ready for game channels to be created by GameMode"));
}

void UVoiceChannelManager::OnGameEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ===== GAME ENDING ====="));
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] Phase: IN-GAME → RETURNING TO LOBBY"));

    CurrentPhase = EGamePhase::Returning;

    UWorld* World = GetWorld();
    if (!World)
        return;

    // 모든 로컬 플레이어의 게임 채널 나가기
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (ATestController* PC = Cast<ATestController>(It->Get()))
        {
            if (PC->IsLocalController())
            {
                PC->LeaveGameChannels();
                UE_LOG(LogTemp, Log, TEXT("[VoiceManager] Local player left game channels"));
            }
        }
    }

    // 잠시 후 로비 페이즈로 전환
    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            CurrentPhase = EGamePhase::Lobby;
            UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] Phase: RETURNING → LOBBY"));
            UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] Players can now use lobby channel again"));
        }, 1.0f, false);
}