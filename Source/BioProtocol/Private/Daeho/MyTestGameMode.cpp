// Fill out your copyright notice in the Description page of Project Settings.


#include "Daeho/MyTestGameMode.h"
#include "Daeho/TestCharacter.h"
#include "Daeho/MyTestController.h"
#include "MyGameStateBase.h"

AMyTestGameMode::AMyTestGameMode()
{
	DefaultPawnClass = ATestCharacter::StaticClass();
	PlayerControllerClass = AMyTestController::StaticClass();

	GameStateClass = AMyGameStateBase::StaticClass();
}

void AMyTestGameMode::RegisterTask(ATaskObject* Task)
{
	// GameState를 가져와서 전체 임무 수 증가
	if (AMyGameStateBase* MS = GetGameState<AMyGameStateBase>())
	{
		MS->TotalTasks++;
	}
}

void AMyTestGameMode::OnTaskCompleted()
{
	if (AMyGameStateBase* MS = GetGameState<AMyGameStateBase>())
	{
		// 진행도 업데이트
		MS->AddCompletedTask();

		// 승리 조건 체크
		if (MS->CompletedTasks >= MS->TotalTasks)
		{
			CivilianWin();
		}
	}
}

void AMyTestGameMode::CivilianWin()
{
	// 여기에 승리 로직 구현 (UI 표시, 게임 재시작 등)
	// 우선 로그로 확인
	UE_LOG(LogTemp, Error, TEXT("모든 임무를 완료했습니다. 시민 승리!!"));

	// 모든 플레이어에게 알림을 보내는 로직을 추가할 수 있습니다.
}
