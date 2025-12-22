// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Equippable/EquippableItem.h"
#include "EquippableTool_Welder.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDurabilityChanged, float, NewDurability);

UCLASS()
class BIOPROTOCOL_API AEquippableTool_Welder : public AEquippableItem
{
	GENERATED_BODY()

public:
	AEquippableTool_Welder();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	virtual void Use() override;
	virtual void StopUsing() override;
	virtual bool CanUse() const override;

	UFUNCTION(BlueprintCallable, Category = "Welder")
	void ConsumeDurability(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Welder")
	void RestoreDurability(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Welder")
	void FullyRecharge();

	UFUNCTION(BlueprintPure, Category = "Welder")
	float GetDurabilityPercent() const { return CurrentDurability / MaxDurability; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Welder|Durability", meta = (ClampMin = "0.0"))
	float MaxDurability;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentDurability, BlueprintReadOnly, Category = "Welder|Durability")
	float CurrentDurability;

	UPROPERTY(EditDefaultsOnly, Category = "Welder|Durability", meta = (ClampMin = "0.0"))
	float DurabilityConsumeRate;

	UPROPERTY(EditDefaultsOnly, Category = "Welder|Durability", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float LowDurabilityThreshold;

	UPROPERTY(BlueprintAssignable, Category = "Item|Durability")
	FOnDurabilityChanged OnDurabilityChanged;

	UPROPERTY(EditDefaultsOnly, Category = "Welder|VFX")
	UParticleSystem* WeldingParticle;

	UPROPERTY()
	UParticleSystemComponent* ActiveWeldingParticle;

	UPROPERTY(EditDefaultsOnly, Category = "Welder|Audio")
	USoundBase* WeldingSound;

	UPROPERTY()
	UAudioComponent* ActiveWeldingSound;

	UFUNCTION()
	void OnRep_CurrentDurability();

	void ProcessDurabilityConsumption(float DeltaTime);

	void StartWeldingEffects();
	void StopWeldingEffects();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerConsumeDurability(float Amount);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRestoreDurability(float Amount);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
