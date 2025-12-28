// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/ItemBase.h"

UItemBase::UItemBase()
{
}

UItemBase* UItemBase::CreateItemCopy() const
{
	UItemBase* ItemCopy = NewObject<UItemBase>(StaticClass());
	
	ItemCopy->ItemID = this->ItemID;
	ItemCopy->Quantity = this->Quantity;
	ItemCopy->DisplayName = this->DisplayName;
	ItemCopy->Rarity = this->Rarity;
	ItemCopy->Category = this->Category;
	ItemCopy->ItemClass = this->ItemClass;
	ItemCopy->EffectRowID = this->EffectRowID;
	ItemCopy->NumericData = this->NumericData;
	ItemCopy->Icon = this->Icon;
	ItemCopy->Mesh = this->Mesh;

	return ItemCopy;

}

void UItemBase::SetQuantity(int NewQuantity)
{
	if(NewQuantity != Quantity)
	{
		Quantity = FMath::Clamp(NewQuantity, 0, NumericData.bIsStackable ? NumericData.MaxStackSize:1);

		/* if (OwningInventory)
		{
			if(Quantity <= 0)
			{
				OwningInventory->RemoveItem(this);
			}
		}*/
	}
}

void UItemBase::UseItem(ADXPlayerCharacter* Character)
{
}
