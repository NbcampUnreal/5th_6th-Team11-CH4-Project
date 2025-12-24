// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MyPlayerController.h"
#include "EngineUtils.h"
#include "Engine/Engine.h" 
#include "Engine/LocalPlayer.h" 

void AMyPlayerController::ClientStartSpectate_Implementation()
{
    StartSpectate();
}

void AMyPlayerController::StartSpectate()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APawn* TargetPawn = nullptr;
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* target = *It;
        if (!target) continue;

        if (target == GetPawn()) continue;

        if (target->IsPlayerControlled())
        {
            TargetPawn = target;
            break;
        }
    }

    if (TargetPawn)
    {
        SetViewTargetWithBlend(TargetPawn, 0.5f);
       
    }
}
