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

	// ?꾨Т 媛앹껜媛 ?ㅽ룿?????몄텧?섏뿬 ?꾩껜 媛쒖닔 利앷?
	void RegisterTask(class ATaskObject* Task);

	// ?꾨Т媛 ?꾨즺?섏뿀?????몄텧?섏뿬 ?밸━ 議곌굔 泥댄겕
	void OnTaskCompleted();

protected:
	// ?쒕? ?밸━ 泥섎━
	void CivilianWin();
};
