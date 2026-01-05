// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "StaffCharacter.h"
#include "Game/BioProtocolTypes.h"
#include "AndroidCharacter.generated.h"

class UPostProcessComponent;
class UNiagaraComponent;
class UAudioComponent;
class ABioGameState;

UCLASS()
class BIOPROTOCOL_API AAndroidCharacter : public AStaffCharacter
{
	GENERATED_BODY()
public:
	AAndroidCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated);
	int8 bHasKilledPlayer = false;

	void UpdateEyeFX(int8 val);

	virtual void EquipSlot1(const FInputActionValue& InValue) override;
	virtual void EquipSlot2(const FInputActionValue& InValue) override;
	virtual void EquipSlot3(const FInputActionValue& InValue) override;
	virtual void InteractPressed(const FInputActionValue& InValue) override;
	UPROPERTY(ReplicatedUsing = OnRep_IsHunter)
	uint8 bIsHunter = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PostProcess")
	UPostProcessComponent* PostProcessComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* AndroidFX;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FX")
	UNiagaraComponent* AndroidFX2;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsHunter();
	void UpdateBreathSound();

	void OnDash();

	void OnChangeMode(float scale);

	UFUNCTION(Server, Reliable)
	void Server_OnChangeMode();

	UFUNCTION(Server, Reliable)
	void Server_DayChangeMode();

	UFUNCTION()
	void OnRep_CharacterScale();

	UFUNCTION(Server, Reliable)
	void Server_Dash();
	UFUNCTION(Client, Reliable)
	void Client_TurnOffXray();

	void Xray();
	UPROPERTY(EditDefaultsOnly, Category = "XRay")
	TSoftObjectPtr<UMaterialInterface> XRayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	//skill(대시,벽투시)용 imc
	TObjectPtr<UInputMappingContext> SkillMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> XrayAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> ScaleChangeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> ChangeAction;

	UPROPERTY()
	USkeletalMeshComponent* ArmsMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AndroidMeleeAttackMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> OriginMeleeAttackMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	float BlinkDistance = 800.f;

	UFUNCTION()

	void OnGamePhaseChanged(EBioGamePhase NewPhase);
	bool IsNightPhase();

	void AndroidArmAttack();

	void SetIsNight(bool val);

	virtual void AttackInput(const FInputActionValue& InValue) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound|Gun")
	USoundBase* AndroidBreathSound;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundAttenuation* BreathAtt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sound")
	UAudioComponent* BreathAudio;

protected:
	virtual void ServerPullLever_Internal() override;
	virtual void PullLever() override;

private:
	int8 bIsXray = false;

	/// <summary>
	/// 퍼지모드 크기변경관련
	/// </summary>
	UPROPERTY(ReplicatedUsing = OnRep_CharacterScale)
	float CharacterScale = 1.f;
private:
	float BaseCapsuleRadius;
	float BaseCapsuleHalfHeight;
	FVector BaseMeshScale;
	FVector BaseCameraOffset;
	FVector BaseMeshOffset;
	float NormalScale = 1.0f;
	float HunterScale = 1.5f;

	TWeakObjectPtr<ABioGameState> CachedGameState;


};