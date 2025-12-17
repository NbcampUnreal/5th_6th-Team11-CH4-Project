#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "DXPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;

USTRUCT()
struct FInteractionData
{
	GENERATED_USTRUCT_BODY()

	//방금 찾은 상호 작용 가능 요소 저장 -> 유효성 확인
	FInteractionData() : CurrentInteractable(nullptr), LastInteractionCheckTime(0.f)
	{

	};
		UPROPERTY()
		AActor* CurrentInteractable;

		UPROPERTY()
		float LastInteractionCheckTime;
	
};

UCLASS()
class BIOPROTOCOL_API ADXPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

#pragma region ACharacter Override

public:
	
	ADXPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion

#pragma region Interaction
public:
	//시간 제한이 있는 상호작용을 위한 함수
	FORCEINLINE bool IsInteracting() const { return GetWorldTimerManager().IsTimerActive(TimerHandle_Interaction); };

protected:
	UPROPERTY(VisibleAnywhere, Category = "Character | IInteractio")
	TScriptInterface<IInteractionInterface> TargetInteractable;

	float InteractionCheckFrequency = 0.02f;

	float InteractionCheckDistance;

	FTimerHandle TimerHandle_Interaction;

	FInteractionData InteractionData;

	//라인트레이스 생성 함수
	void PerformInteractionCheck();
	//새로운 상호작용 가능한 객체와 못 찾음
	void FoundInteractable(AActor* NewInteractable);
	void NoInteractableFound();
	//상호 작용 시작과 끝
	void BeginInteract();
	void EndInteract();
	//캐릭터가 상호작용 동작 호출을 언제 할 지
	void Interact();



#pragma endregion

#pragma region DXPlayerCharacter Components
public:
	FORCEINLINE USpringArmComponent* GetSpringArm() const { return SpringArm; }
	FORCEINLINE UCameraComponent* GetCamera() const { return Camera; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Components")
	TObjectPtr<UCameraComponent> Camera;

#pragma endregion

#pragma region Input
private:
	void HandleMoveInput(const FInputActionValue& InValue);
	void HandleLookInput(const FInputActionValue& InValue);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DXPlayerCharacter|Input")
	TObjectPtr<UInputAction> JumpAction;

#pragma endregion
};
