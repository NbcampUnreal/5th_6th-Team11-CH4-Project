#include "VoiceChannelManager.h"
#include "Character/BioPlayerController.h"
#include "Engine/World.h"

void UVoiceChannelManager::RegisterLobbyChannel(const FString& ChannelName)
{
    LobbyChannelName = ChannelName;
    UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Lobby channel registered"));
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
                    //PC->LeaveLobbyChannel();
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

    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            CurrentPhase = EGamePhase::Lobby;
            UE_LOG(LogTemp, Warning, TEXT("[VoiceManager] ✓ Phase: RETURNING → LOBBY"));
        }, 1.0f, false);
}