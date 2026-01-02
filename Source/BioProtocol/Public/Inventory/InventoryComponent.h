// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BioProtocol/public/World/ItemDataStructs.h"
#include "InventoryComponent.generated.h"


class UItemBase;
class AEquippableItem;
class AStaffCharacter;

//==========================================
// ENUMS
//==========================================

UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NoItemsAdded      UMETA(DisplayName = "No Items Added"),       // 인벤토리 가득참
	IAR_PartialItemsAdded UMETA(DisplayName = "Partial Items Added"),  // 일부만 추가됨
	IAR_AllItemsAdded     UMETA(DisplayName = "All Items Added")       // 전부 추가됨
};

//==========================================
// STRUCTS
//==========================================

USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()

	/** 작업 결과 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	EItemAddResult OperationResult;

	/** 결과 메시지 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	FText ResultMessage;

	/** 실제로 추가된 수량 */
	UPROPERTY(BlueprintReadOnly, Category = "Item Add Result")
	int32 ActualAmountAdded;

	// 기본 생성자
	FItemAddResult()
		: OperationResult(EItemAddResult::IAR_NoItemsAdded)
		, ResultMessage(FText::GetEmpty())
		, ActualAmountAdded(0)
	{
	}

	// 정적 팩토리 메서드
	static FItemAddResult AddedNone(const FText& ErrorMessage)
	{
		FItemAddResult Result;
		Result.OperationResult = EItemAddResult::IAR_NoItemsAdded;
		Result.ResultMessage = ErrorMessage;
		Result.ActualAmountAdded = 0;
		return Result;
	}

	static FItemAddResult AddedPartial(int32 Amount, const FText& Message)
	{
		FItemAddResult Result;
		Result.OperationResult = EItemAddResult::IAR_PartialItemsAdded;
		Result.ResultMessage = Message;
		Result.ActualAmountAdded = Amount;
		return Result;
	}

	static FItemAddResult AddedAll(int32 Amount, const FText& Message)
	{
		FItemAddResult Result;
		Result.OperationResult = EItemAddResult::IAR_AllItemsAdded;
		Result.ResultMessage = Message;
		Result.ActualAmountAdded = Amount;
		return Result;
	}
};

//==========================================
// INVENTORY COMPONENT
//==========================================

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BIOPROTOCOL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	//==========================================
	// INVENTORY PROPERTIES
	//==========================================

	/** 인벤토리 아이템 배열 */
	UPROPERTY(ReplicatedUsing = OnRep_InventoryItems, BlueprintReadOnly, Category = "Inventory")
	TArray<UItemBase*> InventoryItems;

	/** 인벤토리 최대 슬롯 수 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 InventorySlotCapacity;

	/** 인벤토리 최대 무게 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	float InventoryWeightCapacity;

	/** 현재 인벤토리 무게 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	float CurrentInventoryWeight;

	/** 장착 가능한 액터 스폰 */
	AEquippableItem* SpawnEquippableActor(UItemBase* Item);

	//==========================================
	// SLOT SYSTEM 
	//==========================================

	/** 슬롯 1: 주무기 */
	UPROPERTY(ReplicatedUsing = OnRep_Slot1, BlueprintReadOnly, Category = "Inventory|Slots")
	UItemBase* Slot1_Weapon;

	/** 슬롯 2: 업무 도구 */
	UPROPERTY(ReplicatedUsing = OnRep_Slot2, BlueprintReadOnly, Category = "Inventory|Slots")
	UItemBase* Slot2_Tool;

	/** 슬롯 3: 유틸리티 */
	UPROPERTY(ReplicatedUsing = OnRep_Slot3, BlueprintReadOnly, Category = "Inventory|Slots")
	UItemBase* Slot3_Utility;

	/** 현재 장착된 아이템 액터 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentEquippedItemActor, BlueprintReadOnly, Category = "Inventory|Slots")
	AEquippableItem* CurrentEquippedItemActor;

	//==========================================
	// ADD/REMOVE ITEMS
	//==========================================

	/** 아이템 추가 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult HandleAddItem(UItemBase* ItemToAdd);

	/** 아이템 제거 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItemBase* ItemToRemove);

	/** 아이템 제거 (수량 지정) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemByQuantity(UItemBase* ItemToRemove, int32 Quantity);

	//==========================================
	// EQUIP SYSTEM
	//==========================================

	/** 아이템 장착 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void EquipItem(UItemBase* ItemToEquip);

	/** 아이템 해제 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UnequipCurrentItem();

	/** 슬롯에 아이템 배치 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AssignItemToSlot(UItemBase* Item, int32 SlotNumber);

	/** 슬롯에서 아이템 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItemBase* GetItemInSlot(int32 SlotNumber) const;

	//==========================================
	// QUERIES
	//==========================================

	/** 특정 아이템 찾기 (ID로) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItemBase* FindItemByID(FName ItemID) const;

	/** 아이템이 인벤토리에 있는지 확인 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(UItemBase* Item) const;

	/** 인벤토리 공간 확인 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasSpaceForItem(UItemBase* Item) const;

	/** 무게 확인 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool CanAddWeight(float WeightToAdd) const;

	/** 전체 아이템 가져오기 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<UItemBase*>& GetAllItems() const { return InventoryItems; }

	//==========================================
	// UTILITY
	//==========================================

	/** 인벤토리 무게 계산 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CalculateTotalWeight();

	/** 빈 슬롯 개수 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEmptySlotCount() const;

	/** 슬롯 가득 찼는지 확인 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryFull() const;

	//==========================================
	// EVENTS
	//==========================================

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryUpdated OnInventoryUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAdded, UItemBase*, Item, int32, Quantity);
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAdded OnItemAdded;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemoved, UItemBase*, Item, int32, Quantity);
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemoved OnItemRemoved;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemEquipped, UItemBase*, Item);
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemEquipped OnItemEquipped;

protected:
	//==========================================
	// INTERNAL FUNCTIONS
	//==========================================

	/** 스택 가능한 아이템 찾기 */
	UItemBase* FindStackableItem(UItemBase* ItemToAdd) const;

	/** 새 아이템 슬롯 추가 */
	FItemAddResult AddNewItem(UItemBase* ItemToAdd, int32 Quantity);

	/** 기존 스택에 추가 */
	int32 AddToExistingStack(UItemBase* ExistingItem, int32 QuantityToAdd);

	/** 슬롯 자동 배치 */
	void AutoAssignToSlot(UItemBase* Item);

	//==========================================
	// REPLICATION
	//==========================================

	UFUNCTION()
	void OnRep_InventoryItems();

	UFUNCTION()
	void OnRep_Slot1();

	UFUNCTION()
	void OnRep_Slot2();

	UFUNCTION()
	void OnRep_Slot3();

	UFUNCTION()
	void OnRep_CurrentEquippedItemActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

//==========================================
// WEAPON-SPECIFIC FUNCTIONS
//==========================================

/** 인벤토리의 모든 무기 찾기 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	TArray<UItemBase*> GetAllWeaponItems() const;

	/** 특정 카테고리의 아이템 찾기 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UItemBase*> GetItemsByCategory(EItemCategory Category) const;

	/** 인벤토리의 무기 탄약 전부 보급 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	int32 RefillAllWeaponAmmo();

	/** 특정 무기 탄약 보급 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Weapon")
	bool RefillWeaponAmmo(UItemBase* WeaponItem);


private:
	/** 소유 캐릭터 캐시 */
	UPROPERTY()
	AStaffCharacter* OwnerCharacter;
	void CheckWeightLimit();
	void SwapSlots(int32 Slot1, int32 Slot2);
};