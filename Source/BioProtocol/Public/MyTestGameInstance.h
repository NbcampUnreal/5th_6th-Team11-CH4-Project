// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TESTPlayerState.h"
#include "MyTestGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API UMyTestGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // 플레이어 ID별 팀 저장
    TMap<FString, EVoiceTeam> PlayerTeamMap;

    // 팀 저장
    void SavePlayerTeam(const FString& PlayerID, EVoiceTeam Team)
    {
        PlayerTeamMap.Add(PlayerID, Team);
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Saved %s → %s"),
            *PlayerID, Team == EVoiceTeam::Mafia ? TEXT("MAFIA") : TEXT("CITIZEN"));
    }

    // 팀 불러오기
    EVoiceTeam GetPlayerTeam(const FString& PlayerID, bool& bFound)
    {
        if (EVoiceTeam* Team = PlayerTeamMap.Find(PlayerID))
        {
            bFound = true;
            return *Team;
        }
        bFound = false;
        return EVoiceTeam::Citizen;
    }
};