// PickUp.cpp
#include "BioProtocol/Public/World/PickUp.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Character/StaffCharacter.h"

APickUp::APickUp()
{
	PrimaryActorTick.bCanEverTick = false;

	// 네트워크 복제 활성화 (필수!)
	bReplicates = true;
	SetReplicateMovement(true);

	// 픽업 메시 생성
	PickUpMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickUpMesh"));
	PickUpMesh->SetSimulatePhysics(true);
	PickUpMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickUpMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(PickUpMesh);

	// 기본값 초기화
	ItemQuantity = 1;
	ItemReference = nullptr;
}

void APickUp::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 아이템 초기화
	if (HasAuthority())
	{
		InitializePickup(UItemBase::StaticClass(), ItemQuantity);
	}
}

void APickUp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// ItemReference를 모든 클라이언트에 복제
	DOREPLIFETIME(APickUp, ItemReference);
}

void APickUp::InitializePickup(const TSubclassOf<UItemBase> BaseClass, const int32 InItemQuantity)
{
	// 데이터 테이블이 없거나 ID가 유효하지 않으면 종료
	if (!ItemDataTable || DesiredItemID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] ItemDataTable or DesiredItemID is invalid!"));
		return;
	}

	// 데이터 테이블에서 아이템 정보 가져오기
	const FItemData* ItemDataRow = ItemDataTable->FindRow<FItemData>(DesiredItemID, DesiredItemID.ToString());
	if (!ItemDataRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] Failed to find ItemData for ID: %s"), *DesiredItemID.ToString());
		return;
	}

	// UItemBase 객체 생성
	ItemReference = NewObject<UItemBase>(this, BaseClass);
	if (!ItemReference)
	{
		UE_LOG(LogTemp, Error, TEXT("[PickUp] Failed to create ItemReference!"));
		return;
	}

	// 데이터 테이블 값으로 초기화
	ItemReference->ItemID = ItemDataRow->ItemID;
	ItemReference->ItemType = ItemDataRow->ItemType;
	ItemReference->ItemQuality = ItemDataRow->ItemQuality;
	ItemReference->NumericData = ItemDataRow->NumericData;
	ItemReference->TextData = ItemDataRow->TextData;
	ItemReference->AssetData = ItemDataRow->AssetData;  // AssetData 전체 복사
	//ItemReference->ItemClass = ItemDataRow->ItemClass;

	// 수량 설정 (0 이하면 1로 설정)
	const int32 FinalQuantity = FMath::Max(InItemQuantity, 1);
	ItemReference->SetQuantity(FinalQuantity);

	// 메시 설정 ? AssetData.Mesh 사용
	if (PickUpMesh && ItemDataRow->AssetData.Mesh)
	{
		PickUpMesh->SetStaticMesh(ItemDataRow->AssetData.Mesh);
	}

	// 상호작용 데이터 갱신
	UpdateInteractableData();

	UE_LOG(LogTemp, Log, TEXT("[PickUp] Initialized: %s (Quantity: %d)"),
		*ItemReference->TextData.Name.ToString(), FinalQuantity);
}

void APickUp::InitializeDrop(UItemBase* ItemToDrop, const int32 InItemQuantity)
{
	// 유효성 검사
	if (!ItemToDrop)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] ItemToDrop is nullptr!"));
		return;
	}

	// 버린 아이템을 그대로 참조
	ItemReference = ItemToDrop;

	// 수량 설정
	const int32 FinalQuantity = FMath::Max(InItemQuantity, 1);
	ItemReference->SetQuantity(FinalQuantity);

	// 메시 설정
	if (PickUpMesh && ItemToDrop->AssetData.Mesh)
	{
		PickUpMesh->SetStaticMesh(ItemToDrop->AssetData.Mesh);
	}

	// 상호작용 데이터 갱신
	UpdateInteractableData();

	UE_LOG(LogTemp, Log, TEXT("[PickUp] Drop initialized: %s (Quantity: %d)"),
		*ItemReference->TextData.Name.ToString(), FinalQuantity);
}

void APickUp::UpdateInteractableData()
{
	if (!ItemReference)
	{
		return;
	}

	// 상호작용 데이터 구성
	InstanceInteractableData.InteractableType = EInteractableType::Pickup;
	InstanceInteractableData.Action = ItemReference->TextData.InteractionText;
	InstanceInteractableData.Name = ItemReference->TextData.Name;
	InstanceInteractableData.Quantity = ItemReference->Quantity;

	// 인터페이스 데이터 갱신
	InteractableData = InstanceInteractableData;
}

//==========================================
// INTERACTION INTERFACE IMPLEMENTATION
//==========================================

void APickUp::BeginFocus_Implementation()
{
	if (PickUpMesh)
	{
		PickUpMesh->SetRenderCustomDepth(true);
	}
}

void APickUp::EndFocus_Implementation()
{
	if (PickUpMesh)
	{
		PickUpMesh->SetRenderCustomDepth(false);
	}
}

void APickUp::BeginInteract_Implementation()
{
	//진행바, 사운드뭐 그런거?
}

void APickUp::EndInteract_Implementation()
{
	//위의 효과 중단
}


void APickUp::Interact_Implementation(AStaffCharacter* PlayerCharacter)
{
	if (PlayerCharacter)
	{
		TakePickup(PlayerCharacter);
	}
}

FInteractableData APickUp::GetInteractableData_Implementation() const
{
	return InstanceInteractableData;
}

bool APickUp::CanInteract_Implementation(AStaffCharacter* PlayerCharacter) const
{
	// 아이템 참조가 유효하고, 상호작용 가능 상태인지 확인
	return ItemReference != nullptr && InstanceInteractableData.bCanInteract;
}

void APickUp::TakePickup(const AStaffCharacter* Taker)
{
	if (!Taker || !ItemReference || IsPendingKillPending())
	{
		return;
	}

	UInventoryComponent* PlayerInventory = Taker->GetInventory();
	if (!PlayerInventory)
	{
		return;
	}

	// 인벤토리에 아이템 추가 시도
	FItemAddResult AddResult = PlayerInventory->HandleAddItem(ItemReference);

	switch (AddResult.OperationResult)
	{
	case EItemAddResult::IAR_AllItemsAdded:
	{
		// 전부 추가됨 → 픽업 파괴
		UE_LOG(LogTemp, Log, TEXT("[PickUp] All items added to inventory"));

		if (HasAuthority())
		{
			Destroy();
		}
		break;
	}

	case EItemAddResult::IAR_PartialItemsAdded:
	{
		// 일부만 추가됨 → 남은 수량으로 갱신
		const int32 RemainingQuantity = ItemReference->Quantity - AddResult.ActualAmountAdded;
		ItemReference->SetQuantity(RemainingQuantity);
		UpdateInteractableData();

		UE_LOG(LogTemp, Log, TEXT("[PickUp] Partial add. Remaining: %d"), RemainingQuantity);
		break;
	}

	case EItemAddResult::IAR_NoItemsAdded:
	{
		// 추가 안됨 (인벤토리 가득참 or 무게 초과)
		UE_LOG(LogTemp, Warning, TEXT("[PickUp] Cannot add items: %s"),
			*AddResult.ResultMessage.ToString());

		break;
	}
	}
}

//==========================================
// 5. 인벤토리 공간 확인
//==========================================

void AChest::Interact_Implementation(AStaffCharacter* Player)
{
	if (!Player)
	{
		return;
	}

	UInventoryComponent* PlayerInventory = Player->GetInventory();
	if (!PlayerInventory)
	{
		return;
	}
	// ... 아이템 로드

	for (UItemBase* Item : ChestItems)
	{
		if (!Item)
			continue;

		// 공간 확인
		if (PlayerInventory->HasSpaceForItem(Item))
		{
			FItemAddResult Result = PlayerInventory->HandleAddItem(Item);

			if (Result.OperationResult == EItemAddResult::IAR_AllItemsAdded)
			{
				UE_LOG(LogTemp, Log, TEXT("[Chest] Player took: %s"),
					*Item->TextData.Name.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Chest] No space for: %s"),
				*Item->TextData.Name.ToString());
			// UI: "인벤토리 공간이 부족합니다"
		}
	}
}


/* 미션 오브젝트에서 사용
void AMissionObjectBase::Interact_Implementation(AStaffCharacter* Player)
{
	// 플레이어가 필요한 도구를 가지고 있는지 확인
	if (!Player->HasRequiredTool(RequiredToolID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Mission] Player doesn't have required tool: %s"),
			*RequiredToolID.ToString());

		// UI: "렌치가 필요합니다"
		return;
	}

	// 수리 시작
	StartRepair(Player);
}*/

//==========================================
// EDITOR ONLY
//==========================================

#if WITH_EDITOR
void APickUp::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName ChangedPropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// DesiredItemID가 변경되면 메시 자동 업데이트
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(APickUp, DesiredItemID))
	{
		if (ItemDataTable && !DesiredItemID.IsNone())
		{
			const FItemData* ItemData = ItemDataTable->FindRow<FItemData>(
				DesiredItemID,
				DesiredItemID.ToString()
			);

			if (ItemData && PickUpMesh)
			{
				PickUpMesh->SetStaticMesh(ItemData->AssetData.Mesh);
				UE_LOG(LogTemp, Log, TEXT("[PickUp Editor] Mesh updated for: %s"),
					*DesiredItemID.ToString());
			}
		}
	}
}
#endif