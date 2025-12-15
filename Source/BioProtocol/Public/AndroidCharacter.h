// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "StaffCharacter.h"
#include "AndroidCharacter.generated.h"

/**

 *

 */

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
};