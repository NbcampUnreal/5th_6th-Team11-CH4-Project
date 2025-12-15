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
	float Attack = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float Defense = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	float MoveSpeed = 600.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP, BlueprintReadOnly) // <--- ReplicatedUsing Ãß°¡
		float CurrentHP;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentMoveSpeed;

	UPROPERTY(Replicated, BlueprintReadOnly)
	float CurrentAttack;
public:
	void ApplyBaseStatus();
	void ApplyDamage(float Damage);
	bool IsDead() const { return CurrentHP <= 0.f; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_LifeState, BlueprintReadOnly, Category = "Status")
	ECharacterLifeState LifeState = ECharacterLifeState::Alive;

	UFUNCTION()
	void OnRep_LifeState();
	UFUNCTION()
	void OnRep_CurrentHP();
};
