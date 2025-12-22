// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableUtility/EquippableUtility.h"
#include "EquippableUtility_AmmoPack.generated.h"

class AEquippableWeapon_;
/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API AEquippableUtility_AmmoPack : public AEquippableUtility
{
	GENERATED_BODY()

public:
	AEquippableUtility_AmmoPack();

protected:
	//==========================================
	// AMMO REFILL
	//==========================================

	virtual void ProcessUtilityEffect() override;

	/** 모든 무기 보급 여부 (false면 현재 장착된 무기만) */
	UPROPERTY(EditDefaultsOnly, Category = "AmmoPack|Settings")
	bool bRefillAllWeapons;

	/** 보급 이펙트 */
	UPROPERTY(EditDefaultsOnly, Category = "AmmoPack|VFX")
	UParticleSystem* RefillParticle;

private:
	/** 현재 장착된 무기 보급 */
	bool RefillCurrentWeapon();

	/** 인벤토리의 모든 무기 보급 */
	int32 RefillAllWeaponsInInventory();

	/** 무기 장착된 액터들 찾기 */
	TArray<class AEquippableWeapon_*> FindAllEquippedWeapons();
};
