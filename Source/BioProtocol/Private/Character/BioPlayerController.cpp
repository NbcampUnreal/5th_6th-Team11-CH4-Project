// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/BioPlayerController.h"
#include "UI/BioPlayerHUD.h"
#include "Blueprint/UserWidget.h"
#include "Game/BioPlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "IOnlineSubsystemEOS.h"
#include "EngineUtils.h"
#include "EOSVoiceChat.h"
#include "EOSVoiceChatTypes.h"

void ABioPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (BioHUDClass)
		{
			InGameHUD = CreateWidget<UBioPlayerHUD>(this, BioHUDClass);

			if (InGameHUD)
			{
				InGameHUD->AddToViewport();
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("BioHUDClass is NOT set in BioPlayerController!"));
		}

		StartProximityVoice();
	}
}

void ABioPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(ProximityTimer);

    if (VoiceChatUser && IsLocalController())
    {
        if (!PublicChannelName.IsEmpty()) VoiceChatUser->LeaveChannel(PublicChannelName, FOnVoiceChatChannelLeaveCompleteDelegate());
        if (!CleanerChannelName.IsEmpty()) VoiceChatUser->LeaveChannel(CleanerChannelName, FOnVoiceChatChannelLeaveCompleteDelegate());
    }

    Super::EndPlay(EndPlayReason);
}

void ABioPlayerController::CacheVoiceChatUser()
{
    if (VoiceChatUser) return;

    IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld(), TEXT("EOS"));
    if (OSS)
    {
        IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
        if (Identity.IsValid())
        {
            TSharedPtr<const FUniqueNetId> LocalUserId = Identity->GetUniquePlayerId(0);
            if (LocalUserId.IsValid())
            {
                IOnlineSubsystemEOS* EOS = static_cast<IOnlineSubsystemEOS*>(OSS);
                VoiceChatUser = EOS->GetVoiceChatUserInterface(*LocalUserId);
            }
        }
    }
}

void ABioPlayerController::StartProximityVoice()
{
    GetWorldTimerManager().SetTimer(ProximityTimer, this, &ABioPlayerController::UpdateProximityVoice, 0.1f, true);
}

void ABioPlayerController::UpdateProximityVoice()
{
    if (!IsLocalController()) return;
    CacheVoiceChatUser();
    if (!VoiceChatUser || PublicChannelName.IsEmpty()) return;

    FVector MyLoc, MyFwd, MyUp;
    if (GetPawn())
    {
        MyLoc = GetPawn()->GetActorLocation();
        MyFwd = GetPawn()->GetActorForwardVector();
        MyUp = GetPawn()->GetActorUpVector();
    }
    else return;

    VoiceChatUser->Set3DPosition(PublicChannelName, MyLoc, MyLoc, MyFwd, MyUp);

    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* OtherPawn = *It;
        if (OtherPawn == GetPawn()) continue;

        ABioPlayerState* OtherPS = Cast<ABioPlayerState>(OtherPawn->GetPlayerState());
        if (!OtherPS || OtherPS->EOSPlayerName.IsEmpty()) continue;

        float Dist = FVector::Dist(MyLoc, OtherPawn->GetActorLocation());
        float Volume = CalcProxVolume01(Dist, 500.0f, 2500.0f); // 최소/최대 거리 설정

        VoiceChatUser->SetPlayerVolume(OtherPS->EOSPlayerName, Volume);
    }
}

void ABioPlayerController::Client_JoinGameChannel_Implementation(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken)
{
    if (!IsLocalController()) return;
    CacheVoiceChatUser();
    if (!VoiceChatUser) return;

    EVoiceChatChannelType ChannelType = EVoiceChatChannelType::NonPositional;
    if (ChannelName.Contains(TEXT("Public")))
    {
        PublicChannelName = ChannelName;
        ChannelType = EVoiceChatChannelType::Positional;
    }
    else if (ChannelName.Contains(TEXT("Cleaner")))
    {
        CleanerChannelName = ChannelName;
        ChannelType = EVoiceChatChannelType::NonPositional;
    }

    FEOSVoiceChatChannelCredentials Creds;
    Creds.ClientBaseUrl = ClientBaseUrl;
    Creds.ParticipantToken = ParticipantToken;

    VoiceChatUser->JoinChannel(ChannelName, Creds.ToJson(), ChannelType,
        FOnVoiceChatChannelJoinCompleteDelegate::CreateLambda([](const FString& Channel, const FVoiceChatResult& Result) {
            if (Result.IsSuccess()) UE_LOG(LogTemp, Log, TEXT("Joined Channel: %s"), *Channel);
            })
    );
}

float ABioPlayerController::CalcProxVolume01(float Dist, float MinD, float MaxD)
{
    if (Dist <= MinD) return 1.0f;
    if (Dist >= MaxD) return 0.0f;
    float Alpha = (Dist - MinD) / (MaxD - MinD);
    return FMath::Clamp(1.0f - (Alpha * Alpha), 0.0f, 1.0f);
}