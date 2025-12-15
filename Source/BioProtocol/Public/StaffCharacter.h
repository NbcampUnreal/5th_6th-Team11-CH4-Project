// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "StaffCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class BIOPROTOCOL_API AStaffCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AStaffCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void OnDeath();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;
	
private:
	void HandleMoveInput(const FInputActionValue& InValue);

	void HandleLookInput(const FInputActionValue& InValue);

	void HandleStartRun(const FInputActionValue& InValue);

	void HandleStopRun(const FInputActionValue& InValue);

	void HandleCrouch(const FInputActionValue& InValue);

	void HandleStand(const FInputActionValue& InValue);

	void AttackInput(const FInputActionValue& InValue);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> TestKillAction;
private:
	uint8 bIsRunning : 1;

	UFUNCTION(Server, Reliable)
	void ServerStartRun();

	UFUNCTION(Server, Reliable)
	void ServerStopRun();

	UFUNCTION(Server, Reliable)
	void ServerCrouch();

	UFUNCTION(Server, Reliable)
	void ServerUnCrouch();

	UFUNCTION(Server, Reliable)
	void SetSpeed(float speed);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRPCMeleeAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPCMeleeAttack();

	void PlayMeleeAttackMontage();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	//UFUNCTION(Server, Reliable, WithValidation)
	//void ServerRPCTakeDamage(float DamageAmount);
	void TestHit();
	UFUNCTION(Server, Reliable)
	void Server_TestHit();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UStaffStatusComponent> Status;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* StaffMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* StaffArmMesh;
};
