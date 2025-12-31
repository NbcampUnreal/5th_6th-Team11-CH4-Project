#pragma once

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TestCharacter.generated.h"

// 전방 선언
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ADH_PickupItem;

UENUM(BlueprintType)
enum class EToolType : uint8
{
	None    UMETA(DisplayName = "None (Hands)"),
	Wrench  UMETA(DisplayName = "Wrench"),
	Welder  UMETA(DisplayName = "BlowTorch"),
	Gun UMETA(DisplayName = "Gun")
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
	// 변수 복제 설정을 위한 오버라이드
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float DeltaTime) override;
	// 입력을 바인딩하기 위한 함수 오버라이드
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category = "Tool")
	EToolType GetCurrentActiveTool() const;

	//Item Functions
	bool ServerPickUpItem(EToolType NewItemType, int32 NewDurability);
	void ConsumeToolDurability(int32 Amount);

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
	UInputAction* Slot1Action; // 1번 키 (맨손)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* Slot2Action; // 2번 키 (렌치)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* DropAction;

	/** 현재 장착된 도구 (Replicated되어야 서버와 클라가 같은 값을 가짐) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	bool bIsToolEquipped;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	EToolType InventoryItemType;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	int32 InventoryDurability;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<ADH_PickupItem> PickupItemClass;

	// 입력 처리 함수들
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);

	// 도구 변경 입력 처리
	void OnSlot1(const FInputActionValue& Value);
	void OnSlot2(const FInputActionValue& Value);
	
	void OnDrop(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerSetEquipState(bool bEquip);

	UFUNCTION(Server, Reliable)
	void ServerDropItem();

	UFUNCTION(Server, Reliable)
	void ServerInteract();
};