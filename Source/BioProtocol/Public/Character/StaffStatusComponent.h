// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "StaffStatusComponent.generated.h"

UENUM(BlueprintType)
enum class ECharacterLifeState : uint8
{
	Alive UMETA(DisplayName = "Alive"),
	Dead  UMETA(DisplayName = "Dead")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHPChanged, float, NewHP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, NewStamina);

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
	float Defense = 5.f;

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
	FOnStaminaChanged OnStaminaChanged;

public:
	void ApplyBaseStatus();
	void ApplyDamage(float Damage);
	bool IsDead() const { return CurrentHP <= 0.f; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "Status")
	ECharacterLifeState LifeState = ECharacterLifeState::Alive;

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
	UFUNCTION()
	void OnRep_LifeState();
	UFUNCTION()
	void OnRep_CurrentHP();
	UFUNCTION()
	void OnRep_CurrentStamina();
	void SetRunable() {	bIsRunable = true;};
	

	float GetCurrentHP() { return CurrentHP; }

private:
	FTimerHandle TimerHandle_SetRunable;
};
