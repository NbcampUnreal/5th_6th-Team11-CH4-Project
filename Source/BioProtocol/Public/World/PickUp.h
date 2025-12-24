// PickUp.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BioProtocol/Public/Interfaces/InteractionInterface.h"
#include "BioProtocol/Public/World/ItemDataStructs.h"  // ? ItemDataStructs Include
#include "PickUp.generated.h"

class UItemBase;
class UDataTable;
class ADXPlayerCharacter;

UCLASS()
class BIOPROTOCOL_API APickUp : public AActor, public IInteractionInterface
{
	GENERATED_BODY()

public:
	APickUp();

	//==========================================
	// COMPONENTS
	//==========================================

	UPROPERTY(VisibleAnywhere, Category = "PickUp | Components")
	UStaticMeshComponent* PickUpMesh;

	//==========================================
	// VARIABLES
	//==========================================

	/** 이 픽업이 참조하는 아이템 데이터 테이블 */
	UPROPERTY(EditInstanceOnly, Category = "PickUp | Item Initialization")
	UDataTable* ItemDataTable;

	/** 데이터 테이블에서 가져올 아이템 ID */
	UPROPERTY(EditInstanceOnly, Category = "PickUp | Item Initialization")
	FName DesiredItemID;

	/** 스폰 시 기본 수량 (0이하면 1로 설정됨) */
	UPROPERTY(EditInstanceOnly, Category = "PickUp | Item Initialization", meta = (ClampMin = 0))
	int32 ItemQuantity;

	/** 런타임에 생성된 아이템 참조 */
	UPROPERTY(VisibleAnywhere, Replicated, Category = "PickUp | Item Reference")
	UItemBase* ItemReference;

	/** 상호작용 UI에 표시될 데이터 */
	UPROPERTY(VisibleAnywhere, Category = "PickUp | Interaction")
	FInteractableData InstanceInteractableData;

	//==========================================
	// FUNCTIONS
	//==========================================

	/**
	 * 데이터 테이블에서 아이템 정보를 읽어와 초기화
	 * @param BaseClass - 생성할 UItemBase 클래스
	 * @param InItemQuantity - 초기 수량
	 */
	void InitializePickup(const TSubclassOf<UItemBase> BaseClass, const int32 InItemQuantity);

	/**
	 * 인벤토리에서 버린 아이템을 픽업으로 초기화
	 * @param ItemToDrop - 버릴 아이템
	 * @param InItemQuantity - 수량
	 */
	void InitializeDrop(UItemBase* ItemToDrop, const int32 InItemQuantity);

	/** 상호작용 데이터 갱신 */
	FORCEINLINE void UpdateInteractableData();

	/** 플레이어가 아이템을 습득 */
	void TakePickup(const ADXPlayerCharacter* Taker);

	//==========================================
	// INTERACTION INTERFACE
	//==========================================

	virtual void BeginFocus_Implementation() override;
	virtual void EndFocus_Implementation() override;
	virtual void BeginInteract_Implementation() override;
	virtual void EndInteract_Implementation() override;
	virtual void Interact_Implementation(ADXPlayerCharacter* PlayerCharacter) override;
	virtual FInteractableData GetInteractableData_Implementation() const override;
	virtual bool CanInteract_Implementation(ADXPlayerCharacter* PlayerCharacter) const override;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

UCLASS()
class BIOPROTOCOL_API AChest : public APickUp
{
	GENERATED_BODY()

public:
	virtual void Interact_Implementation(ADXPlayerCharacter* Player) override;

	UPROPERTY(EditAnywhere, Category = "Chest")
	TArray<UItemBase*> ChestItems;
};