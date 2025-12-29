// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableUtility/EquippableUtility.h"
#include "EquippableUtility_SignalJammer.generated.h"

class USphereComponent;
class UParticleSystemComponent;
/**
 * 신호 교란기 - 설치형 디버프 아이템
 * 10초간 작동하며 범위 내 AI의 입력을 반전시킴
 */
UCLASS()
class BIOPROTOCOL_API AEquippableUtility_SignalJammer : public AEquippableUtility
{
	GENERATED_BODY()
	
public:
	AEquippableUtility_SignalJammer();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	//==========================================
	// JAMMER FUNCTIONS
	//==========================================

	virtual void ProcessUtilityEffect() override;

	/** 교란기 작동 중지 */
	UFUNCTION(BlueprintCallable, Category = "SignalJammer")
	void StopJammer();

	//==========================================
	// COMPONENTS
	//==========================================

	/** 효과 범위 (Sphere Collision) */
	UPROPERTY(VisibleAnywhere, Category = "SignalJammer|Components")
	USphereComponent* EffectSphere;

	//==========================================
	// PROPERTIES
	//==========================================

	/** 지속 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "SignalJammer|Settings")
	float Duration;

	/** 효과 범위 (미터) */
	UPROPERTY(EditDefaultsOnly, Category = "SignalJammer|Settings")
	float EffectRadius;

	/** 작동 중인지 여부 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "SignalJammer|State")
	bool bIsActive;

	/** 작동 시작 시간 */
	UPROPERTY()
	float ActivationTime;

	//==========================================
	// VFX
	//==========================================

	UPROPERTY(EditDefaultsOnly, Category = "SignalJammer|VFX")
	UParticleSystem* JammerParticle;

	UPROPERTY()
	UParticleSystemComponent* ActiveJammerParticle;

	//==========================================
	// TIMER
	//==========================================

	FTimerHandle JammerTimerHandle;
	FTimerHandle TickTimerHandle;

protected:
	//==========================================
	// INTERNAL
	//==========================================

	/** 범위 내 AI에게 디버프 적용 */
	void ApplyJammerEffect();

	/** 교란기 이펙트 시작 */
	void StartJammerEffects();

	/** 교란기 이펙트 중지 */
	void StopJammerEffects();

	//==========================================
	// COLLISION CALLBACKS
	//==========================================

	UFUNCTION()
	void OnEffectSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnEffectSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	//==========================================
	// REPLICATION
	//==========================================

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
