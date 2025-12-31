// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Game/BioProtocolTypes.h"
#include "StaffStatusComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHPChanged, float, NewHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NewStamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusChanged, EBioPlayerStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTransformChanged, bool, bIsTransformed);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BIOPROTOCOL_API UStaffStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UStaffStatusComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float Attack = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MoveSpeed = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float RunMultiply = 1.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float StaminaReduce = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float StaminaIncrease = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float JumpStamina = 20.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, BlueprintReadOnly) 
	float CurrentHP;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentStamina;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentMoveSpeed;

	UPROPERTY(Replicated, BlueprintReadOnly)
	uint8 bIsRunable=true;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentAttack;

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnHPChanged OnHPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Status")
	FOnStaminaChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable)
	FOnStatusChanged OnStatusChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTransformChanged OnTransformChanged;

public:
	void ApplyBaseStatus();
	void ApplyDamage(float Damage, AController* DamageInstigator);
	bool IsDead() const { return CurrentHP <= 0.f; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerStatus, BlueprintReadOnly, Category = "Status")
	EBioPlayerStatus PlayerStatus = EBioPlayerStatus::Alive;

	UPROPERTY(ReplicatedUsing = OnRep_IsTransformed, BlueprintReadOnly, Category = "Status")
	bool bIsTransformed = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Status")
	float JailTimer = 0.0f;

	void StartConsumeStamina();
	void StopConsumeStamina();

	void StartRegenStamina();
	void StopRegenStamina();

	void ConsumeStaminaTick();
	void RegenStaminaTick();

	bool CanJumpByStamina() const;
	void ConsumeJumpStamina();

	FTimerHandle TimerHandle_ConsumeStamina;
	FTimerHandle TimerHandle_RegenStamina;
	FTimerHandle TimerHandle_Jail;
	UFUNCTION()
	void OnRep_CurrentHP();
	UFUNCTION()
	void OnRep_CurrentStamina();
	UFUNCTION()
	void OnRep_PlayerStatus();
	UFUNCTION()
	void OnRep_IsTransformed();
	void SetRunable() {	bIsRunable = true;};

	UFUNCTION(Server, Reliable)
	void ServerSetTransform(bool bNewState);
	
	void JailTimerTick();
	void SetJailed();
	void SetDead();
	void SetRevived();

	float GetCurrentHP() { return CurrentHP; }
	void SetFullCurrentHP() { CurrentHP = MaxHP; }

private:
	FTimerHandle TimerHandle_SetRunable;
};
