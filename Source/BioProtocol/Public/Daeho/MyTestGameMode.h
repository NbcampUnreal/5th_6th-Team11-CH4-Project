// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyTestGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API AMyTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyTestGameMode();

	// 임무 객체가 스폰될 때 호출하여 전체 개수 증가
	void RegisterTask(class ATaskObject* Task);

	// 임무가 완료되었을 때 호출하여 승리 조건 체크
	void OnTaskCompleted();

protected:
	// 시민 승리 처리
	void CivilianWin();
};
