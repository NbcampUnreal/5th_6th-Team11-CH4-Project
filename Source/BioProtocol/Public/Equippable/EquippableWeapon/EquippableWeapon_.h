// EquippableWeapon.h
#pragma once

#include "CoreMinimal.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "EquippableWeapon_.generated.h"

class UParticleSystem;
class USoundBase;
class ADXPlayerCharacter;

/**
 * 무기 베이스 클래스
 * 모든 무기(쇠파이프, 권총, 샷건)의 공통 기능 제공
 */
UCLASS(Abstract)
class BIOPROTOCOL_API AEquippableWeapon_ : public AEquippableItem
{
	GENERATED_BODY()

public:
	AEquippableWeapon_();

protected:
	virtual void BeginPlay() override;

public:
	//==========================================
	// WEAPON FUNCTIONS
	//==========================================

	/** 공격 (발사/근접) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Attack();

	/** 재장전 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Reload();

	/** 공격 가능 여부 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool CanAttack() const;

	/** 재장전 가능 여부 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	virtual bool CanReload() const;

	//==========================================
	// OVERRIDE
	//==========================================

	virtual void Use() override;
	virtual void StopUsing() override;
	virtual bool CanUse() const override;

	//==========================================
	// AMMO MANAGEMENT
	//==========================================

	/** 탄약 추가 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	void AddAmmo(int32 Amount);

	/** 탄약 완전 보급 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	void RefillAmmo();

	/** 탄약 비율 가져오기 (0.0 ~ 1.0) */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	float GetAmmoPercent() const;

	//==========================================
	// PROPERTIES - Weapon Stats
	//==========================================

	/** 무기 데미지 */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float Damage;

	/** 사거리 (미터) */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float Range;

	/** 발사 속도 (초당 발사 수) */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	float FireRate;

	/** 근접 무기 여부 */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Stats")
	bool bIsMeleeWeapon;

	//==========================================
	// PROPERTIES - Ammo
	//==========================================

	/** 현재 탄약 (탄창 내) */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 CurrentAmmo;

	/** 최대 탄약 (탄창 크기) */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo")
	int32 MaxAmmo;

	/** 무한 탄약 여부 (근접 무기용) */
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Ammo")
	bool bInfiniteAmmo;

	//==========================================
	// STATE
	//==========================================

	/** 재장전 중인지 여부 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsReloading;

	/** 마지막 발사 시간 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|State")
	float LastAttackTime;

	//==========================================
	// TIMER
	//==========================================

	FTimerHandle ReloadTimerHandle;
	FTimerHandle FireRateTimerHandle;

	//==========================================
	// EVENTS
	//==========================================

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, int32, NewAmmo);
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAmmoChanged OnAmmoChanged;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStarted);
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadStarted OnReloadStarted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadCompleted);
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnReloadCompleted OnReloadCompleted;

protected:
	//==========================================
	// INTERNAL FUNCTIONS
	//==========================================

	/** 실제 공격 처리 (자식 클래스에서 오버라이드) */
	virtual void ProcessAttack();

	/** 재장전 완료 처리 */
	virtual void CompleteReload();

	/** 발사 이펙트 재생 */
	void PlayAttackEffects();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|VFX")
	UParticleSystem* AttackParticle;

	/** 히트 처리 (자식 클래스에서 구현) */
	virtual void ProcessHit(const FHitResult& HitResult);

	//==========================================
	// REPLICATION
	//==========================================

	UFUNCTION()
	void OnRep_CurrentAmmo();

	UFUNCTION(Server, Reliable)
	void ServerAttack();

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayAttackEffects();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};