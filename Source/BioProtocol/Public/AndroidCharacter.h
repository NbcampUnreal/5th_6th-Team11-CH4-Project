// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "StaffCharacter.h"
#include "AndroidCharacter.generated.h"

/**

 *

 */
class UPostProcessComponent;
UCLASS()
class BIOPROTOCOL_API AAndroidCharacter : public AStaffCharacter
{
	GENERATED_BODY()
public:
	AAndroidCharacter();
	void SwitchAndroidMode();
	void SwitchToStaff();
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION()
	void OnRep_IsAndroid();	

protected:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PostProcess")
	UPostProcessComponent* PostProcessComp;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> StaffAnim;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* AndroidMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* AndroidArmMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AndroidAnim;
	UFUNCTION(Server, Reliable)
	void ServerSwitchAndroid();
	UFUNCTION(Server, Reliable)
	void ServerSwitchToStaff();

	void OnDash();

	void OnChangeMode(float scale);

	UFUNCTION(Server, Reliable)
	void Server_OnChangeMode();

	UFUNCTION()
	void OnRep_CharacterScale();

	UFUNCTION(Server, Reliable)
	void Server_Dash();

	void Xray();
	UPROPERTY(EditDefaultsOnly, Category = "XRay")
	TSoftObjectPtr<UMaterialInterface> XRayMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputMappingContext> SkillMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> XrayAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> ScaleChangeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> ChangeAction;
	UPROPERTY(ReplicatedUsing = OnRep_IsAndroid)
	uint8 bIsAndroid : 1;
	UPROPERTY()
	USkeletalMeshComponent* ArmsMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AndroidMeleeAttackMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> OriginMeleeAttackMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blink")
	float BlinkDistance = 800.f;
private:
	int8 bIsXray = false;
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
	UPROPERTY();
	int8 bIsHunter = false;
};