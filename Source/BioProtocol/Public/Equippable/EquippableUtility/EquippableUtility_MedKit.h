// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableUtility/EquippableUtility.h"
#include "EquippableUtility_MedKit.generated.h"

/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API AEquippableUtility_MedKit : public AEquippableUtility
{
	GENERATED_BODY()

public:
	AEquippableUtility_MedKit();

protected:
	//==========================================
	// HEALING
	//==========================================

	virtual void ProcessUtilityEffect() override;

	/** 회복량 */
	UPROPERTY(EditDefaultsOnly, Category = "MedKit|Settings")
	float HealAmount;

	/** 타인 치료 가능 여부 */
	UPROPERTY(EditDefaultsOnly, Category = "MedKit|Settings")
	bool bCanHealOthers;

	/** 타인 치료 범위 */
	UPROPERTY(EditDefaultsOnly, Category = "MedKit|Settings")
	float HealRange;

	/** 자가 치료 */
	void HealSelf();

	/** 타인 치료 */
	void HealOther();

	/** 치료 가능한 타겟 찾기 */
	AStaffCharacter* FindHealTarget();
	
};
