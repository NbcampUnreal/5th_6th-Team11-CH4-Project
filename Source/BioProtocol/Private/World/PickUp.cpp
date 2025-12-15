#include "PickUp.h"


APickUp::APickUp()
{

	PrimaryActorTick.bCanEverTick = false;

	PickUpMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickUpMesh"));
	PickUpMesh->SetSimulatePhysics(true);
	SetRootComponent(PickUpMesh);

}

void APickUp::BeginPlay()
{
	Super::BeginPlay();

	InitializePickup(UItemBase::StaticClass(), ItemQuantity);
}

void APickUp::InitializePickup(const TSubclassOf<UItemBase> Baseclass, const int32 ItemQuantity)
{
	if (ItemDataTable && !DesiredItemID.IsNone())
	{
		const FItemData* ItemDataRow = ItemDataTable->FindRow<FItemData>(DesiredItemID, DesiredItemID.ToString());
		
		ItemReference = NewObject<UItemBase>(this, Baseclass);

        // 내부 ID
        ItemReference->ItemID = ItemDataRow->ItemID;

        // 표시 이름
        ItemReference->DisplayName = ItemDataRow->DisplayName;

        // 희귀도
        ItemReference->Rarity = ItemDataRow->Rarity;

        // 카테고리
        ItemReference->Category = ItemDataRow->Category;

        // 스폰할 액터 클래스
        ItemReference->ItemClass = ItemDataRow->ItemClass;

        // 효과 RowID
        ItemReference->EffectRowID = ItemDataRow->EffectRowID;

        // 공용 숫자 데이터 (무게, 스택 정보 등)
        ItemReference->NumericData = ItemDataRow->NumericData;

		ItemQuantity <= 0 ? ItemReference->SetQuantity(1) : ItemReference->SetQuantity(ItemQuantity);

		PickUpMesh->SetStaticMesh(ItemDataRow->AssetData.Mesh);
		
		UpdateInteractableData();
	}
}

void APickUp::InitializeDrop(UItemBase* ItemToDrop, const int32 ItemQuantity)
{
	ItemReference = ItemToDrop;
	ItemQuantity <= 0 ? ItemReference->SetQuantity(1) : ItemReference->SetQuantity(ItemQuantity);
	ItemReference->NumericData.Weight = ItemToDrop->NumericData.Weight();
	PickUpMesh->SetStaticMesh(ItemToDrop->Asset.Mesh);

	UpdateInteractableData();

}

void APickUp::UpdateInteractableData()
{
	InstanceInteractableData.InteractableType = EInteractableType::Pickup;
	InstanceInteractableData.Action = ItemReference->TextData.InteractionText;
	InstanceInteractableData.Name = ItemReference->TextData.Name;
	InstanceInteractableData.Quantity = ItemReference->Quantity;
	InteractableData = InstanceInteractableData;
}


void APickUp::BeginFocus()
{
	if (PickUpMesh)
	{
		PickUpMesh->SetRenderCustomDepth(true);
	}
}

void APickUp::EndFocus()
{
	if (PickUpMesh)
	{
		PickUpMesh->SetRenderCustomDepth(true);
	}
}

void APickUp::Interact(ADXPlayerCharacter* PlayerCharacter)
{
	if(PlayerCharacter)
	{
		TakePickup(PlayerCharacter);
	}
}

void APickUp::TakePickup(const ADXPlayerCharacter* Taker)
{
	if(IsPendingKillPending())
	{
		return;
	}
}

