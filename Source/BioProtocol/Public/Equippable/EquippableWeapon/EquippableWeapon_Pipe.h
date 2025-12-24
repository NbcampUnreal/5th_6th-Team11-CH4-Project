// EquippableWeapon_Pipe.h
#pragma once

#include "CoreMinimal.h"
#include "BioProtocol/Public/Equippable/EquippableWeapon/EquippableWeapon_.h"
#include "EquippableWeapon_Pipe.generated.h"

/**
 * 쇠파이프 - 근접 무기
 * 박스 트레이스로 앞의 적을 타격
 */
UCLASS()
class BIOPROTOCOL_API AEquippableWeapon_Pipe : public AEquippableWeapon_
{
	GENERATED_BODY()

public:
	AEquippableWeapon_Pipe();

protected:
	//==========================================
	// MELEE ATTACK
	//==========================================

	/** 공격 처리 (BoxTrace) */
	virtual void ProcessAttack() override;

	/** 근접 공격 범위 (박스 크기) */
	UPROPERTY(EditDefaultsOnly, Category = "Pipe|Melee")
	FVector MeleeBoxExtent;

	/** 공격 애니메이션 재생 */
	virtual void PlayMeleeAnimation();

protected:
	// 파이프 근접 공격 몽타주 (에디터에서 할당)
	UPROPERTY(EditDefaultsOnly, Category = "Pipe|Animation")
	UAnimMontage* PipeMeleeMontage;
};