// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TestCharacter.generated.h"

// 전방 선언
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UENUM(BlueprintType)
enum class EToolType : uint8
{
	None    UMETA(DisplayName = "None (Hands)"),
	Wrench  UMETA(DisplayName = "Wrench"),
	Welder  UMETA(DisplayName = "BlowTorch")
};

UCLASS()
class BIOPROTOCOL_API ATestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATestCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// 입력을 바인딩하기 위한 함수 오버라이드
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 변수 복제 설정을 위한 오버라이드
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 현재 장착 중인 도구 (Getter) */
	UFUNCTION(BlueprintCallable, Category = "Tool")
	EToolType GetCurrentTool() const { return CurrentTool; }

protected:
	/** 1인칭 카메라 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* FirstPersonCameraComponent;

	/** [Enhanced Input] 매핑 컨텍스트 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* DefaultMappingContext;

	/** [Enhanced Input] 이동 액션 (WASD) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;

	/** [Enhanced Input] 시선 액션 (Mouse XY) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;

	/** [Enhanced Input] 점프 액션 (Space) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* JumpAction;

	/** [Enhanced Input] 상호작용 액션 (E Key) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* InteractAction;

	/** 도구 변경 액션 (1, 2, 3 Key) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipSlot1Action; // 1번 키 (맨손)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipSlot2Action; // 2번 키 (렌치)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EquipSlot3Action; // 3번 키 (용접기)

	/** 현재 장착된 도구 (Replicated되어야 서버와 클라가 같은 값을 가짐) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Tool")
	EToolType CurrentTool;

	// 입력 처리 함수들
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

	// 도구 변경 입력 처리
	void OnEquipSlot1(const FInputActionValue& Value);
	void OnEquipSlot2(const FInputActionValue& Value);
	void OnEquipSlot3(const FInputActionValue& Value);

	// 서버에 도구 변경 요청 (RPC)
	UFUNCTION(Server, Reliable)
	void ServerEquipTool(EToolType NewTool);

	// 서버로 상호작용 요청 (RPC)
	UFUNCTION(Server, Reliable)
	void ServerInteract();
};