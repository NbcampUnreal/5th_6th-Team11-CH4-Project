// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/InteractionWidget.h"
#include "Components/TextBlock.h"

void UInteractionWidget::ShowInteraction(FString Message)
{
	if (ActionText)
	{
		ActionText->SetText(FText::FromString(Message));
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UInteractionWidget::HideInteraction()
{
	SetVisibility(ESlateVisibility::Hidden);
}
