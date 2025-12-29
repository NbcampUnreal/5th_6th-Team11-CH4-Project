// InventoryComponent.cpp
#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h" 
#include "Net/UnrealNetwork.h"
#include "Character/StaffCharacter.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// 기본 인벤토리 용량
	InventorySlotCapacity = 20;
	InventoryWeightCapacity = 50.0f;
	CurrentInventoryWeight = 0.0f;

	// 슬롯 초기화
	Slot1_Weapon = nullptr;
	Slot2_Tool = nullptr;
	Slot3_Utility = nullptr;
	CurrentEquippedItemActor = nullptr;

	OwnerCharacter = nullptr;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 소유 캐릭터 캐싱
	OwnerCharacter = Cast<AStaffCharacter>(GetOwner());
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, InventoryItems);
	DOREPLIFETIME(UInventoryComponent, Slot1_Weapon);
	DOREPLIFETIME(UInventoryComponent, Slot2_Tool);
	DOREPLIFETIME(UInventoryComponent, Slot3_Utility);
	DOREPLIFETIME(UInventoryComponent, CurrentEquippedItemActor);
}

//==========================================
// ADD ITEM
//==========================================

FItemAddResult UInventoryComponent::HandleAddItem(UItemBase* ItemToAdd)
{

	if (!ItemToAdd)
	{
		return FItemAddResult::AddedNone(FText::FromString("Invalid item"));
	}

	if (!GetOwner()->HasAuthority())
	{
		return FItemAddResult::AddedNone(FText::FromString("Must be called on server"));
	}

	// 무게 확인
	if (!CanAddWeight(ItemToAdd->GetItemStackWeight()))
	{
		return FItemAddResult::AddedNone(FText::FromString("Inventory too heavy"));
	}

	// 아이템 소유권 설정
	ItemToAdd->OwningInventory = this;
	ItemToAdd->SetQuantity(FMath::Max(ItemToAdd->Quantity, 1));

	// 스택 가능한 기존 아이템 찾기
	if (ItemToAdd->IsStackable())
	{
		UItemBase* ExistingItem = FindStackableItem(ItemToAdd);
		if (ExistingItem)
		{
			// 기존 스택에 추가
			int32 AmountAdded = AddToExistingStack(ExistingItem, ItemToAdd->Quantity);

			if (AmountAdded == ItemToAdd->Quantity)
			{
				// 전부 추가됨
				CalculateTotalWeight();
				OnItemAdded.Broadcast(ItemToAdd, AmountAdded);
				OnInventoryUpdated.Broadcast();

				return FItemAddResult::AddedAll(AmountAdded,
					FText::Format(FText::FromString("Added {0} to stack"),
						FText::AsNumber(AmountAdded)));
			}
			else if (AmountAdded > 0)
			{
				// 일부만 추가됨 → 나머지는 새 슬롯에
				int32 Remaining = ItemToAdd->Quantity - AmountAdded;
				ItemToAdd->SetQuantity(Remaining);

				FItemAddResult NewSlotResult = AddNewItem(ItemToAdd, Remaining);

				CalculateTotalWeight();
				OnItemAdded.Broadcast(ItemToAdd, AmountAdded + NewSlotResult.ActualAmountAdded);
				OnInventoryUpdated.Broadcast();

				return FItemAddResult::AddedPartial(
					AmountAdded + NewSlotResult.ActualAmountAdded,
					FText::FromString("Partially stacked"));
			}
		}
	}

	// 새 슬롯에 추가
	return AddNewItem(ItemToAdd, ItemToAdd->Quantity);
}

FItemAddResult UInventoryComponent::AddNewItem(UItemBase* ItemToAdd, int32 Quantity)
{
	if (!ItemToAdd)
	{
		return FItemAddResult::AddedNone(FText::FromString("Invalid item"));
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] AddNewItem called"));
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] Item: %s"),
		*ItemToAdd->TextData.Name.ToString());
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] ItemType: %s"),
		*UEnum::GetValueAsString(ItemToAdd->ItemType));
	UE_LOG(LogTemp, Warning, TEXT("[Inventory] Quantity: %d"), Quantity);

	// ========================================
	// 타입별로 정해진 슬롯에 할당
	// ========================================

	UItemBase** TargetSlot = nullptr;
	int32 SlotNumber = 0;
	FString SlotName;

	switch (ItemToAdd->ItemType)
	{
	case EItemType::Weapon:
		if (Slot1_Weapon)
		{
			UE_LOG(LogTemp, Error, TEXT("[Inventory] Weapon slot is already occupied by: %s"),
				*Slot1_Weapon->TextData.Name.ToString());
			return FItemAddResult::AddedNone(FText::FromString("Weapon slot is full"));
		}
		TargetSlot = &Slot1_Weapon;
		SlotNumber = 1;
		SlotName = TEXT("Weapon");
		break;

	case EItemType::Tool:
		if (Slot2_Tool)
		{
			UE_LOG(LogTemp, Error, TEXT("[Inventory] Tool slot is already occupied by: %s"),
				*Slot2_Tool->TextData.Name.ToString());
			return FItemAddResult::AddedNone(FText::FromString("Tool slot is full"));
		}
		TargetSlot = &Slot2_Tool;
		SlotNumber = 2;
		SlotName = TEXT("Tool");
		break;

	case EItemType::Utility:
	case EItemType::Consumable:
		if (Slot3_Utility)
		{
			UE_LOG(LogTemp, Error, TEXT("[Inventory] Utility slot is already occupied by: %s"),
				*Slot3_Utility->TextData.Name.ToString());
			return FItemAddResult::AddedNone(FText::FromString("Utility slot is full"));
		}
		TargetSlot = &Slot3_Utility;
		SlotNumber = 3;
		SlotName = TEXT("Utility");
		break;

	default:
		UE_LOG(LogTemp, Error, TEXT("[Inventory] Unknown item type: %s"),
			*UEnum::GetValueAsString(ItemToAdd->ItemType));
		return FItemAddResult::AddedNone(FText::FromString("Unknown item type"));
	}

	// ========================================
	// 슬롯에 할당
	// ========================================
	*TargetSlot = ItemToAdd;

	UE_LOG(LogTemp, Warning, TEXT("[Inventory] ? Auto-assigned to Slot %d (%s): %s"),
		SlotNumber, *SlotName, *ItemToAdd->TextData.Name.ToString());

	// ========================================
	// 무게 계산
	// ========================================
	CalculateTotalWeight();

	// ========================================
	// 이벤트 브로드캐스트
	// ========================================
	OnItemAdded.Broadcast(ItemToAdd, Quantity);
	OnInventoryUpdated.Broadcast();

	UE_LOG(LogTemp, Warning, TEXT("========================================"));

	// ========================================
	// 성공 리턴
	// ========================================
	FString ResultMessage = FString::Printf(TEXT("Added %s to Slot %d"),
		*ItemToAdd->TextData.Name.ToString(), SlotNumber);

	return FItemAddResult::AddedAll(
		Quantity,  // ← 파라미터로 받은 Quantity 사용!
		FText::FromString(ResultMessage)
	);
}

int32 UInventoryComponent::AddToExistingStack(UItemBase* ExistingItem, int32 QuantityToAdd)
{
	if (!ExistingItem || !ExistingItem->IsStackable())
	{
		return 0;
	}

	// 스택에 추가 가능한 수량 계산
	int32 MaxStackSize = ExistingItem->GetMaxStackSize();
	int32 CurrentQuantity = ExistingItem->Quantity;
	int32 AvailableSpace = MaxStackSize - CurrentQuantity;

	if (AvailableSpace <= 0)
	{
		return 0;
	}

	// 추가
	int32 AmountToAdd = FMath::Min(QuantityToAdd, AvailableSpace);
	ExistingItem->SetQuantity(CurrentQuantity + AmountToAdd);

	UE_LOG(LogTemp, Log, TEXT("[Inventory] Added %d to existing stack: %s (Total: %d/%d)"),
		AmountToAdd, *ExistingItem->TextData.Name.ToString(),
		ExistingItem->Quantity, MaxStackSize);

	return AmountToAdd;
}

//==========================================
// REMOVE ITEM
//==========================================

bool UInventoryComponent::RemoveItem(UItemBase* ItemToRemove)
{
	if (!ItemToRemove || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (InventoryItems.Remove(ItemToRemove) > 0)
	{
		// 슬롯에서도 제거
		if (Slot1_Weapon == ItemToRemove) Slot1_Weapon = nullptr;
		if (Slot2_Tool == ItemToRemove) Slot2_Tool = nullptr;
		if (Slot3_Utility == ItemToRemove) Slot3_Utility = nullptr;

		// 무게 재계산
		CalculateTotalWeight();

		// 이벤트
		OnItemRemoved.Broadcast(ItemToRemove, ItemToRemove->Quantity);
		OnInventoryUpdated.Broadcast();

		UE_LOG(LogTemp, Log, TEXT("[Inventory] Removed item: %s"),
			*ItemToRemove->TextData.Name.ToString());

		return true;
	}

	return false;
}

bool UInventoryComponent::RemoveItemByQuantity(UItemBase* ItemToRemove, int32 Quantity)
{
	if (!ItemToRemove || Quantity <= 0 || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (ItemToRemove->Quantity <= Quantity)
	{
		// 전부 제거
		return RemoveItem(ItemToRemove);
	}
	else
	{
		// 일부만 제거
		ItemToRemove->SetQuantity(ItemToRemove->Quantity - Quantity);
		CalculateTotalWeight();

		OnItemRemoved.Broadcast(ItemToRemove, Quantity);
		OnInventoryUpdated.Broadcast();

		UE_LOG(LogTemp, Log, TEXT("[Inventory] Removed %d from item: %s (Remaining: %d)"),
			Quantity, *ItemToRemove->TextData.Name.ToString(), ItemToRemove->Quantity);

		return true;
	}
}

//==========================================
// 6. 무게 제한 체크
//==========================================

void UInventoryComponent::CheckWeightLimit()
{
	if (CurrentInventoryWeight > InventoryWeightCapacity)
	{
		UE_LOG(LogTemp, Error, TEXT("[Inventory] Overweight! %.1f / %.1f"),
			CurrentInventoryWeight, InventoryWeightCapacity);

		// 이동 속도 감소
		if (OwnerCharacter)
		{
			UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement();
			if (Movement)
			{
				float OverweightPercent = CurrentInventoryWeight / InventoryWeightCapacity;
				float SpeedMultiplier = FMath::Clamp(2.0f - OverweightPercent, 0.5f, 1.0f);

				Movement->MaxWalkSpeed = 600.0f * SpeedMultiplier;
			}
		}
	}
}

//==========================================
// 8. 슬롯 재배치
//==========================================

void UInventoryComponent::SwapSlots(int32 Slot1, int32 Slot2)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UItemBase* Item1 = GetItemInSlot(Slot1);
	UItemBase* Item2 = GetItemInSlot(Slot2);

	if (Item1)
	{
		AssignItemToSlot(Item1, Slot2);
	}

	if (Item2)
	{
		AssignItemToSlot(Item2, Slot1);
	}

	OnInventoryUpdated.Broadcast();
}

//==========================================
// EQUIP SYSTEM
//==========================================

void UInventoryComponent::EquipItem(UItemBase* ItemToEquip)
{
	if (!ItemToEquip || !OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] Cannot equip - invalid item or owner"));
		return;
	}

	// 서버에 요청
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// 현재 장착된 아이템 해제
	if (CurrentEquippedItemActor)
	{
		CurrentEquippedItemActor->Unequip();
		CurrentEquippedItemActor = nullptr;
	}

	// 새 아이템 스폰 및 장착
	AEquippableItem* SpawnedItem = SpawnEquippableActor(ItemToEquip);
	if (SpawnedItem)
	{
		SpawnedItem->Initialize(ItemToEquip, OwnerCharacter);
		SpawnedItem->Equip();

		CurrentEquippedItemActor = SpawnedItem;
		OwnerCharacter->CurrentEquippedItem = SpawnedItem;

		OnItemEquipped.Broadcast(ItemToEquip);

		UE_LOG(LogTemp, Log, TEXT("[Inventory] Equipped: %s"),
			*ItemToEquip->TextData.Name.ToString());
	}
}

void UInventoryComponent::UnequipCurrentItem()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (CurrentEquippedItemActor)
	{
		CurrentEquippedItemActor->Unequip();
		CurrentEquippedItemActor->Destroy();
		CurrentEquippedItemActor = nullptr;

		if (OwnerCharacter)
		{
			OwnerCharacter->CurrentEquippedItem = nullptr;
		}

		UE_LOG(LogTemp, Log, TEXT("[Inventory] Unequipped item"));
	}
}

AEquippableItem* UInventoryComponent::SpawnEquippableActor(UItemBase* Item)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !Item || !Item->ItemClass)
	{
		return nullptr;
	}

	if (!OwnerActor->HasAuthority())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEquippableItem* SpawnedItem = GetWorld()->SpawnActor<AEquippableItem>(
		Item->ItemClass,
		OwnerCharacter->GetActorLocation(),
		OwnerCharacter->GetActorRotation(),
		SpawnParams
	);

	return SpawnedItem;
}

//==========================================
// SLOT SYSTEM
//==========================================

bool UInventoryComponent::AssignItemToSlot(UItemBase* Item, int32 SlotNumber)
{
	if (!Item || !GetOwner()->HasAuthority())
	{
		return false;
	}

	switch (SlotNumber)
	{
	case 1:
		// 무기만 슬롯 1에
		if (Item->Category == EItemCategory::Weapon)
		{
			Slot1_Weapon = Item;
			return true;
		}
		break;

	case 2:
		// 도구만 슬롯 2에
		if (Item->Category == EItemCategory::Work)
		{
			Slot2_Tool = Item;
			return true;
		}
		break;

	case 3:
		// 유틸리티만 슬롯 3에
		if (Item->Category == EItemCategory::Utility)
		{
			Slot3_Utility = Item;
			return true;
		}
		break;

	default:
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Inventory] Cannot assign %s to slot %d - wrong category"),
		*Item->TextData.Name.ToString(), SlotNumber);

	return false;
}

void UInventoryComponent::AutoAssignToSlot(UItemBase* Item)
{
	if (!Item)
	{
		return;
	}

	// 카테고리에 따라 자동 배치
	switch (Item->Category)
	{
	case EItemCategory::Weapon:
		if (!Slot1_Weapon)
		{
			Slot1_Weapon = Item;
			UE_LOG(LogTemp, Log, TEXT("[Inventory] Auto-assigned to Slot 1: %s"),
				*Item->TextData.Name.ToString());
		}
		break;

	case EItemCategory::Work:
		if (!Slot2_Tool)
		{
			Slot2_Tool = Item;
			UE_LOG(LogTemp, Log, TEXT("[Inventory] Auto-assigned to Slot 2: %s"),
				*Item->TextData.Name.ToString());
		}
		break;

	case EItemCategory::Utility:
		if (!Slot3_Utility)
		{
			Slot3_Utility = Item;
			UE_LOG(LogTemp, Log, TEXT("[Inventory] Auto-assigned to Slot 3: %s"),
				*Item->TextData.Name.ToString());
		}
		break;
	}
}

UItemBase* UInventoryComponent::GetItemInSlot(int32 SlotNumber) const
{
	switch (SlotNumber)
	{
	case 1: return Slot1_Weapon;
	case 2: return Slot2_Tool;
	case 3: return Slot3_Utility;
	default: return nullptr;
	}
}

//==========================================
// QUERIES
//==========================================

UItemBase* UInventoryComponent::FindItemByID(FName ItemID) const
{
	for (UItemBase* Item : InventoryItems)
	{
		if (Item && Item->ItemID == ItemID)
		{
			return Item;
		}
	}
	return nullptr;
}

bool UInventoryComponent::HasItem(UItemBase* Item) const
{
	return InventoryItems.Contains(Item);
}

bool UInventoryComponent::HasSpaceForItem(UItemBase* Item) const
{
	if (!Item)
	{
		return false;
	}

	// 무게 확인
	if (!CanAddWeight(Item->GetItemStackWeight()))
	{
		return false;
	}

	// 스택 가능하면 기존 아이템 확인
	if (Item->IsStackable())
	{
		UItemBase* ExistingItem = FindStackableItem(Item);
		if (ExistingItem)
		{
			// 기존 스택에 공간이 있으면 OK
			if (ExistingItem->Quantity < ExistingItem->GetMaxStackSize())
			{
				return true;
			}
		}
	}

	// 새 슬롯 필요 → 빈 슬롯 확인
	return !IsInventoryFull();
}

bool UInventoryComponent::CanAddWeight(float WeightToAdd) const
{
	return (CurrentInventoryWeight + WeightToAdd) <= InventoryWeightCapacity;
}

UItemBase* UInventoryComponent::FindStackableItem(UItemBase* ItemToAdd) const
{
	if (!ItemToAdd || !ItemToAdd->IsStackable())
	{
		return nullptr;
	}

	for (UItemBase* Item : InventoryItems)
	{
		if (Item &&
			Item->ItemID == ItemToAdd->ItemID &&
			!Item->IsFullStack())
		{
			return Item;
		}
	}

	return nullptr;
}

//==========================================
// WEAPON
//==========================================
TArray<UItemBase*> UInventoryComponent::GetAllWeaponItems() const
{
	TArray<UItemBase*> WeaponItems;

	for (UItemBase* Item : InventoryItems)
	{
		if (Item && Item->Category == EItemCategory::Weapon)
		{
			WeaponItems.Add(Item);
		}
	}

	return WeaponItems;
}

TArray<UItemBase*> UInventoryComponent::GetItemsByCategory(EItemCategory Category) const
{
	TArray<UItemBase*> CategoryItems;

	for (UItemBase* Item : InventoryItems)
	{
		if (Item && Item->Category == Category)
		{
			CategoryItems.Add(Item);
		}
	}

	return CategoryItems;
}

int32 UInventoryComponent::RefillAllWeaponAmmo()
{
	if (!GetOwner()->HasAuthority())
	{
		return 0;
	}

	int32 RefillCount = 0;
	TArray<UItemBase*> WeaponItems = GetAllWeaponItems();

	for (UItemBase* WeaponItem : WeaponItems)
	{
		if (RefillWeaponAmmo(WeaponItem))
		{
			RefillCount++;
		}
	}

	if (RefillCount > 0)
	{
		OnInventoryUpdated.Broadcast();
	}

	UE_LOG(LogTemp, Log, TEXT("[Inventory] Refilled ammo for %d weapons"), RefillCount);

	return RefillCount;
}

bool UInventoryComponent::RefillWeaponAmmo(UItemBase* WeaponItem)
{
	if (!WeaponItem || WeaponItem->Category != EItemCategory::Weapon)
	{
		return false;
	}

	// 이미 풀탄창이면 보급 안함
	if (WeaponItem->CurrentAmmo >= WeaponItem->MaxAmmo)
	{
		return false;
	}

	// 탄약 보급
	int32 AmmoBeforeRefill = WeaponItem->CurrentAmmo;
	WeaponItem->RefillAmmo();

	UE_LOG(LogTemp, Log, TEXT("[Inventory] Refilled weapon ammo: %s (%d -> %d)"),
		*WeaponItem->TextData.Name.ToString(),
		AmmoBeforeRefill,
		WeaponItem->CurrentAmmo);

	return true;
}

//==========================================
// UTILITY
//==========================================

void UInventoryComponent::CalculateTotalWeight()
{
	CurrentInventoryWeight = 0.0f;

	for (UItemBase* Item : InventoryItems)
	{
		if (Item)
		{
			CurrentInventoryWeight += Item->GetItemStackWeight();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Inventory] Total weight: %.1f / %.1f"),
		CurrentInventoryWeight, InventoryWeightCapacity);
}

int32 UInventoryComponent::GetEmptySlotCount() const
{
	return InventorySlotCapacity - InventoryItems.Num();
}

bool UInventoryComponent::IsInventoryFull() const
{
	return InventoryItems.Num() >= InventorySlotCapacity;
}

//==========================================
// REPLICATION CALLBACKS
//==========================================

void UInventoryComponent::OnRep_InventoryItems()
{
	UE_LOG(LogTemp, Warning, TEXT("[Inventory Client] Items count: %d"), InventoryItems.Num());

	OnInventoryUpdated.Broadcast();
}

void UInventoryComponent::OnRep_Slot1()
{
	UE_LOG(LogTemp, Warning, TEXT("[Inventory Client] Slot 1 updated: %s"),
		Slot1_Weapon ? *Slot1_Weapon->TextData.Name.ToString() : TEXT("EMPTY"));

	// 현재 슬롯이 1이면 다시 장착
	if (OwnerCharacter && OwnerCharacter->CurrentSlot == 1)
	{
		if (Slot1_Weapon)
		{
			UE_LOG(LogTemp, Log, TEXT("[Inventory Client] Re-equipping Slot 1 item"));
			EquipItem(Slot1_Weapon);
		}
	}
}

void UInventoryComponent::OnRep_Slot2()
{
	UE_LOG(LogTemp, Warning, TEXT("[Inventory Client] Slot 2 updated: %s"),
		Slot2_Tool ? *Slot2_Tool->TextData.Name.ToString() : TEXT("EMPTY"));

	// 현재 슬롯이 2이면 다시 장착
	if (OwnerCharacter && OwnerCharacter->CurrentSlot == 2)
	{
		if (Slot2_Tool)
		{
			UE_LOG(LogTemp, Log, TEXT("[Inventory Client] Re-equipping Slot 2 item"));
			EquipItem(Slot2_Tool);
		}
	}
}

void UInventoryComponent::OnRep_Slot3()
{
	UE_LOG(LogTemp, Warning, TEXT("[Inventory Client] Slot 3 updated: %s"),
		Slot3_Utility ? *Slot3_Utility->TextData.Name.ToString() : TEXT("EMPTY"));

	// 현재 슬롯이 3이면 다시 장착
	if (OwnerCharacter && OwnerCharacter->CurrentSlot == 3)
	{
		if (Slot3_Utility)
		{
			UE_LOG(LogTemp, Log, TEXT("[Inventory Client] Re-equipping Slot 3 item"));
			EquipItem(Slot3_Utility);
		}
	}
}

void UInventoryComponent::OnRep_CurrentEquippedItemActor()
{
	UE_LOG(LogTemp, Warning, TEXT("[Inventory Client] OnRep_CurrentEquippedItemActor "));

	if (CurrentEquippedItemActor)
	{
		UE_LOG(LogTemp, Log, TEXT("[Inventory Client] CurrentEquippedItemActor: %s"),
			*CurrentEquippedItemActor->GetName());

		// 클라이언트에서도 장착 처리
		if (OwnerCharacter)
		{
			OwnerCharacter->CurrentEquippedItem = CurrentEquippedItemActor;
			
			CurrentEquippedItemActor->PerformAttachment(CurrentEquippedItemActor->HandSocketName);
			CurrentEquippedItemActor->bIsEquipped = true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Inventory Client] CurrentEquippedItemActor: NULL"));

		// 장착 해제
		if (OwnerCharacter)
		{
			OwnerCharacter->CurrentEquippedItem = nullptr;
		}
	}
}

//==========================================
// SERVER RPC
//==========================================
/*
void UInventoryComponent::ServerEquipItem_Implementation(UItemBase* Item)
{
	EquipItem(Item);
}

void UInventoryComponent::ServerUnequipItem_Implementation()
{
	UnequipCurrentItem();
}
*/