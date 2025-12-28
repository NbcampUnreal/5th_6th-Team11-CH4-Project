// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TESTPlayerState.h"
#include "Game/BioProtocolTypes.h"
#include "MyTestGameInstance.generated.h"



UCLASS()
class BIOPROTOCOL_API UMyTestGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    TMap<FString, EBioPlayerRole> PlayerRoleMap;

    // 역할 저장
    void SavePlayerRole(const FString& PlayerID, EBioPlayerRole Role)
    {
        PlayerRoleMap.Add(PlayerID, Role);

        FString RoleText;
        switch (Role)
        {
        case EBioPlayerRole::None: RoleText = TEXT("NONE"); break;
        case EBioPlayerRole::Staff: RoleText = TEXT("STAFF"); break;
        case EBioPlayerRole::Cleaner: RoleText = TEXT("CLEANER"); break;
        default: RoleText = TEXT("UNKNOWN"); break;
        }

        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Saved %s → %s"), *PlayerID, *RoleText);
    }

    // 역할 불러오기
    EBioPlayerRole GetPlayerRole(const FString& PlayerID, bool& bFound)
    {
        if (EBioPlayerRole* Role = PlayerRoleMap.Find(PlayerID))
        {
            bFound = true;
            return *Role;
        }
        bFound = false;
        return EBioPlayerRole::None;
    }
};