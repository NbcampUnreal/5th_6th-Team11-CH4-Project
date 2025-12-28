// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableUtility/EquippableUtility.h"
#include "EquippableUtility_SignalJammer.generated.h"

/**
 * 신호 교란기 - 설치형 디버프 아이템
 * 10초간 작동하며 범위 내 AI의 입력을 반전시킴
 */
UCLASS()
class BIOPROTOCOL_API AEquippableUtility_SignalJammer : public AEquippableUtility
{
	GENERATED_BODY()
	
};
