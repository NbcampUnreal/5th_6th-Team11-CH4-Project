// Fill out your copyright notice in the Description page of Project Settings.


#include "TestGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "../Session/SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"

void ATestGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS) return;

    IOnlineSessionPtr Session = OSS->GetSessionInterface();
    if (!Session.IsValid()) return;

    APlayerState* PS = NewPlayer->GetPlayerState<APlayerState>();
    if (!PS || !PS->GetUniqueId().IsValid()) return;

    const FUniqueNetIdRepl& IdRepl = PS->GetUniqueId();
    Session->RegisterPlayer(
        NAME_GameSession,
        *IdRepl.GetUniqueNetId(),
        /*bWasInvited=*/ false
    );
}

void ATestGameMode::Logout(AController* Exiting)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (OSS)
    {
        IOnlineSessionPtr Session = OSS->GetSessionInterface();
        if (Session.IsValid())
        {
            APlayerState* PS = Exiting->GetPlayerState<APlayerState>();
            if (PS && PS->GetUniqueId().IsValid())
            {
                const FUniqueNetIdRepl& IdRepl = PS->GetUniqueId();
                Session->UnregisterPlayer(
                    NAME_GameSession,
                    *IdRepl.GetUniqueNetId()
                );
            }
        }
    }

    Super::Logout(Exiting);
}