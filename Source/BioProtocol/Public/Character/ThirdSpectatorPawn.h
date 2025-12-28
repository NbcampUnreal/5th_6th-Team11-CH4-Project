// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "ThirdSpectatorPawn.generated.h"

class UInputAction;
class UInputMappingContext;

//class FInputActionValue;

UCLASS()
class BIOPROTOCOL_API AThirdSpectatorPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AThirdSpectatorPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void HandleLookInput(const FInputActionValue& InValue);
public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 관전 대상 설정 함수
	void SetSpectateTarget(AActor* NewTarget);
	UFUNCTION(Server, Reliable)
	void Server_SpectateNextPlayer();
	UFUNCTION(Client, Reliable)
	void Client_SetSpectateTarget(ACharacter* NewTarget);
	// 다음 플레이어 관전 (입력 바인딩용)
	void SpectateNextPlayer();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> NextAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* SceneRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FollowCamera;

	// 현재 관전 중인 대상
	UPROPERTY(Replicated)
		AActor* CurrentTarget;
};
