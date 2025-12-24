// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameModeBase.h"
#include "Character/StaffCharacter.h"
#include <Character/ThirdSpectatorPawn.h>

void AMyGameModeBase::OnPlayerKilled(AController* VictimController)
{
    if (!VictimController) return;

    APawn* KilledPawn = VictimController->GetPawn();
    if (KilledPawn)
    {
        KilledPawn->Destroy(); 
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = VictimController;

    FVector SpawnLoc = KilledPawn ? KilledPawn->GetActorLocation() : FVector::ZeroVector;
    FRotator SpawnRot = FRotator::ZeroRotator;

    AThirdSpectatorPawn* Spectator = GetWorld()->SpawnActor<AThirdSpectatorPawn>(
        SpectatorPawnClass,
        SpawnLoc,
        SpawnRot,
        SpawnParams
    );

    if (Spectator)
    {
        VictimController->Possess(Spectator);

        VictimController->SetControlRotation(Spectator->GetActorRotation());

        if (APlayerController* PC = Cast<APlayerController>(VictimController))
        {
            PC->SetInputMode(FInputModeGameOnly());
            PC->bShowMouseCursor = false;
        }

        Spectator->SpectateNextPlayer();
    }
   
}

void AMyGameModeBase::RestartPlayer(AController* NewPlayer)
{
    Super::RestartPlayer(NewPlayer);

    if (AStaffCharacter* NewChar = Cast<AStaffCharacter>(NewPlayer->GetPawn()))
    {       
        NewChar->SetMaterialByIndex(AssignedMaterialCount);
        ++AssignedMaterialCount;
    }
}