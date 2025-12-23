// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TESTPlayerState.generated.h"

UENUM(BlueprintType)
enum class EVoiceTeam : uint8
{
	Citizen,
	Mafia
};

UCLASS()
class BIOPROTOCOL_API ATESTPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_VoiceTeam, Category = "Voice")
    EVoiceTeam VoiceTeam = EVoiceTeam::Citizen;

    UFUNCTION()
    void OnRep_VoiceTeam();

    // ✅ 블루프린트에서 호출
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Voice")
    void Server_RequestTeamChange(EVoiceTeam NewTeam);

    void Server_SetVoiceTeam(EVoiceTeam NewTeam);

    UPROPERTY(ReplicatedUsing = OnRep_EOSPlayerName, BlueprintReadOnly, Category = "EOS")
    FString EOSPlayerName;

    void Server_SetEOSPlayerName(const FString& InEOSPlayerName);

    UPROPERTY(BlueprintReadWrite, Replicated)
    bool bisReady = false;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_EOSPlayerName();

protected:
    virtual void BeginPlay() override;

private:
    void TryInitEOSPlayerName();
    FTimerHandle EOSNameInitTimer;

    // ✅ 팀 복원 함수
    void RestoreTeamFromGameInstance();
};
