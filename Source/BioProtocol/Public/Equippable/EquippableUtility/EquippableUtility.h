// EquippableUtility.h
#pragma once

#include "CoreMinimal.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "Delegates/DelegateCombinations.h"
#include "EquippableUtility.generated.h"

class AStaffCharacter;

//==========================================
// EVENTS
//==========================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUsesChanged, int32, RemainingUses);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUtilityDepleted);

/**
 * 유틸리티 베이스 클래스
 * 소모품 아이템 (회복키트, 탄약팩, 신호교란기)
 */
UCLASS(Abstract)
class BIOPROTOCOL_API AEquippableUtility : public AEquippableItem
{
	GENERATED_BODY()

public:
	AEquippableUtility();

protected:
	virtual void BeginPlay() override;

public:
	//==========================================
	// UTILITY FUNCTIONS
	//==========================================

	/** 유틸리티 사용 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	virtual void UseUtility();

	/** 사용 가능 여부 */
	UFUNCTION(BlueprintPure, Category = "Utility")
	virtual bool CanUseUtility() const;

	//==========================================
	// OVERRIDE
	//==========================================

	virtual void Use() override;
	virtual bool CanUse() const override;

	//==========================================
	// PROPERTIES
	//==========================================

	/** 소모품 여부 (사용 후 삭제) */
	UPROPERTY(EditDefaultsOnly, Category = "Utility|Settings")
	bool bConsumable;

	/** 사용 횟수 (0이면 무제한) */
	UPROPERTY(ReplicatedUsing = OnRep_UsesRemaining, BlueprintReadOnly, Category = "Utility|Settings")
	int32 UsesRemaining;

	/** 최대 사용 횟수 */
	UPROPERTY(EditDefaultsOnly, Category = "Utility|Settings")
	int32 MaxUses;

	/** 사용 쿨다운 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Utility|Settings")
	float Cooldown;

	/** 마지막 사용 시간 */
	UPROPERTY()
	float LastUseTime;

	/** 쿨다운 중인지 여부 */
	UPROPERTY(BlueprintReadOnly, Category = "Utility|State")
	bool bIsOnCooldown;

	//==========================================
	// VFX & AUDIO
	//==========================================

	UPROPERTY(EditDefaultsOnly, Category = "Utility|VFX")
	UParticleSystem* UseParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Utility|Audio")
	USoundBase* UseSound;

protected:
	//==========================================
	// INTERNAL
	//==========================================

	/** 실제 유틸리티 효과 (자식 클래스에서 구현) */
	virtual void ProcessUtilityEffect();

	/** 사용 횟수 감소 */
	void ConsumeUse();

	/** 쿨다운 체크 */
	bool IsInCooldown() const;

	/** 사용 이펙트 재생 */
	void PlayUseEffects();

	//==========================================
	// REPLICATION
	//==========================================

	UFUNCTION()
	void OnRep_UsesRemaining();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayUseEffects();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Utility|Events")
	FOnUsesChanged OnUsesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Utility|Events")
	FOnUtilityDepleted OnUtilityDepleted;
};