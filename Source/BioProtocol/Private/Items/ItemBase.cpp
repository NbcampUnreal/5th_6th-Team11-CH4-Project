// ItemBase.cpp
#include "BioProtocol/Public/Items/ItemBase.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "Net/UnrealNetwork.h"

UItemBase::UItemBase()
{
	ItemID = NAME_None;
	Rarity = EItemRarity::Low;
	Category = EItemCategory::Work;
	Quantity = 1;
	ItemClass = nullptr;
	EffectRowID = NAME_None;
	OwningInventory = nullptr;

	// 무기용 데이터 초기화
	CurrentAmmo = 0;
	MaxAmmo = 0;
}

void UItemBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemBase, ItemID);
	DOREPLIFETIME(UItemBase, Rarity);
	DOREPLIFETIME(UItemBase, Category);
	DOREPLIFETIME(UItemBase, Quantity);
	DOREPLIFETIME(UItemBase, NumericData);
	DOREPLIFETIME(UItemBase, TextData);
	DOREPLIFETIME(UItemBase, AssetData);
	DOREPLIFETIME(UItemBase, ItemClass);
	DOREPLIFETIME(UItemBase, EffectRowID);
	DOREPLIFETIME(UItemBase, CurrentAmmo);
}

UWorld* UItemBase::GetWorld() const
{
	if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}
	return nullptr;
}

void UItemBase::SetQuantity(const int32 NewQuantity)
{
	// 0 ~ MaxStackSize 사이로 클램프
	Quantity = FMath::Clamp(NewQuantity, 0, NumericData.MaxStackSize);

	// 수량이 0이 되면 인벤토리에서 제거(인벤토리 컴포넌트에서 처리)
	if (Quantity <= 0 && OwningInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemBase] %s quantity is 0, should be removed"),
			*TextData.Name.ToString());
	}
}

UItemBase* UItemBase::CreateItemCopy() const
{
	// 같은 클래스의 새 인스턴스 생성
	UItemBase* ItemCopy = NewObject<UItemBase>(GetOuter(), StaticClass());

	// 모든 데이터 복사
	ItemCopy->ItemID = this->ItemID;
	ItemCopy->Rarity = this->Rarity;
	ItemCopy->Category = this->Category;
	ItemCopy->Quantity = this->Quantity;
	ItemCopy->NumericData = this->NumericData;
	ItemCopy->TextData = this->TextData;
	ItemCopy->AssetData = this->AssetData;
	// ItemCopy->ItemClass = this->ItemClass;
	ItemCopy->EffectRowID = this->EffectRowID;
	// ItemCopy->OwningInventory = nullptr; // 새 아이템이므로 소유자 없음

	return ItemCopy;
}

void UItemBase::Use(AStaffCharacter* Character)
{
	// 기본 구현: 아무것도 하지 않음
	// 자식 클래스에서 오버라이드해서 사용
	UE_LOG(LogTemp, Warning, TEXT("[ItemBase] Use() called but not implemented for: %s"),
		*TextData.Name.ToString());
}

#if WITH_EDITOR
void UItemBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// 수량 변경 시 유효성 검사
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UItemBase, Quantity))
	{
		Quantity = FMath::Clamp(Quantity, 1, NumericData.MaxStackSize);
	}
}
#endif

void UItemBase::SetAmmo(int32 NewAmmo)
{
	CurrentAmmo = FMath::Clamp(NewAmmo, 0, MaxAmmo);
}

void UItemBase::AddAmmo(int32 Amount)
{
	CurrentAmmo = FMath::Clamp(CurrentAmmo + Amount, 0, MaxAmmo);
}

void UItemBase::RefillAmmo()
{
	CurrentAmmo = MaxAmmo;
}

float UItemBase::GetAmmoPercent() const
{
	if (MaxAmmo <= 0)
	{
		return 0.0f;
	}
	return static_cast<float>(CurrentAmmo) / static_cast<float>(MaxAmmo);
}