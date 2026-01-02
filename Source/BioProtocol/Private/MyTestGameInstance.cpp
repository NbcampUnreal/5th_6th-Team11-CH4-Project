// Fill out your copyright notice in the Description page of Project Settings.


#include "MyTestGameInstance.h"


void UMyTestGameInstance::SavePlayerColor(FString PlayerName, int32 ColorIdx)
{
    if (PlayerName.IsEmpty()) return;
    PlayerColorMap.Add(PlayerName, ColorIdx);
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("색깔 선택!"));
}
int32 UMyTestGameInstance::GetPlayerColor(FString PlayerName, bool& bFound)
{
    if (PlayerColorMap.Contains(PlayerName))
    {
        bFound = true;
        return PlayerColorMap[PlayerName];
    }
    bFound = false;
    return 0;
}