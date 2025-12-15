// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameModeBase.h"
#include <ThirdSpectatorPawn.h>

void AMyGameModeBase::OnPlayerKilled(AController* VictimController)
{
    if (!VictimController) return;

    // 1. 기존 캐릭터 처리 (Ragdoll 혹은 Destroy)
    APawn* KilledPawn = VictimController->GetPawn();
    if (KilledPawn)
    {
        KilledPawn->Destroy(); // 혹은 래그돌 처리 후 UnPossess
    }

    // 2. 관전용 폰 생성
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = VictimController;

    // 위치는 사망한 자리에서 시작
    FVector SpawnLoc = KilledPawn ? KilledPawn->GetActorLocation() : FVector::ZeroVector;
    FRotator SpawnRot = FRotator::ZeroRotator;

    AThirdSpectatorPawn* Spectator = GetWorld()->SpawnActor<AThirdSpectatorPawn>(
        AThirdSpectatorPawn::StaticClass(),
        SpawnLoc,
        SpawnRot,
        SpawnParams
    );

    // 3. 컨트롤러 빙의 (Possess)
    if (Spectator)
    {
        VictimController->Possess(Spectator);

        // [추가] 컨트롤러의 회전 값을 초기화하거나 동기화
        // 관전 폰은 Controller의 회전을 따르도록 설정했으므로, 
        // 컨트롤러가 엉뚱한 곳을 보고 있거나 회전이 잠겨있으면 안 됩니다.
        VictimController->SetControlRotation(Spectator->GetActorRotation());

        // [중요] 혹시 사망 처리 중에 입력 모드가 UI Only로 바뀌었는지 확인
        if (APlayerController* PC = Cast<APlayerController>(VictimController))
        {
            // 다시 게임 모드로 설정 (마우스 커서 숨김, 입력 활성화)
            PC->SetInputMode(FInputModeGameOnly());
            PC->bShowMouseCursor = false;
        }

        Spectator->SpectateNextPlayer();
    }
   
}
