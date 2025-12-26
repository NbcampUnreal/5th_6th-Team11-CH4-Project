// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "VoiceChat.h"
#include "BioPlayerController.generated.h"

class UBioPlayerHUD;

UCLASS()
class BIOPROTOCOL_API ABioPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Client, Reliable)
	void Client_JoinGameChannel(const FString& ChannelName, const FString& ClientBaseUrl, const FString& ParticipantToken);

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UBioPlayerHUD> BioHUDClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UBioPlayerHUD* InGameHUD;

protected:
    void StartProximityVoice();
    void UpdateProximityVoice();
    void CacheVoiceChatUser();

    float CalcProxVolume01(float Dist, float MinD, float MaxD);

    IVoiceChatUser* VoiceChatUser = nullptr;
    FTimerHandle ProximityTimer;

    FString PublicChannelName;
    FString CleanerChannelName;
};