// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/IsolationTimerWidget.h"
#include "Components/TextBlock.h"

void UIsolationTimerWidget::UpdateTime(float TimeRemaining)
{
	if (TimeText)
	{
		int32 IntTime = FMath::CeilToInt(TimeRemaining);

		TimeText->SetText(FText::AsNumber(IntTime));
	}
}