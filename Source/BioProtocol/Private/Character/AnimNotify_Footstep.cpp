// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AnimNotify_Footstep.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

void UAnimNotify_Footstep::Notify(
    USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation
)
{
    if (!MeshComp || !FootstepSound)
        return;

    APawn* Pawn = Cast<APawn>(MeshComp->GetOwner());
    if (!Pawn)
        return;

    if (Pawn->GetLocalRole() != ROLE_AutonomousProxy)
        return;

    UGameplayStatics::PlaySoundAtLocation(
        Pawn,
        FootstepSound,
        MeshComp->GetComponentLocation()
    );
}