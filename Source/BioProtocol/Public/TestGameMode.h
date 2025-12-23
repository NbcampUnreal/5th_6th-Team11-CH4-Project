// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TESTPlayerState.h"
#include "TestGameMode.generated.h"


UCLASS()
class BIOPROTOCOL_API ATestGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

public:
    // 특정 팀을 위한 프라이빗 채널 생성
    UFUNCTION(BlueprintCallable, Category = "Voice")
    void CreatePrivateVoiceChannel(EVoiceTeam Team, const TArray<APlayerController*>& Players);

private:
    void OnPrivateChannelCreated(bool bSuccess, const FString& Response, EVoiceTeam Team, const TArray<APlayerController*>& Players);

    void TryCreateVoiceChannels();

    FString TrustedServerUrl = TEXT("http://localhost:3000");
	
};
