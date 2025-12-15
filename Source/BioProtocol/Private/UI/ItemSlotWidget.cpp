// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ItemSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UItemSlotWidget::UpdateSlot(UTexture2D* Icon)
{
	if (ItemIcon)
	{
		if (Icon)
		{
			ItemIcon->SetBrushFromTexture(Icon);
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
