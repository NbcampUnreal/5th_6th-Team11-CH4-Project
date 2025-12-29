// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Interaction/MissionObjectBase.h"
#include "DentaentionLever.generated.h"

/**
 * 격리실 레버 - 격리실 문을 여는 미션 오브젝트
 */

UCLASS()
class BIOPROTOCOL_API ADentaentionLever : public AMissionObjectBase
{
	GENERATED_BODY()
	
public:
	//ADentaentionLever();

protected:
	virtual void BeginPlay() override;

};
