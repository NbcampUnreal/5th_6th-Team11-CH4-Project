// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SkillSlotWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Math/UnrealMathUtility.h"

void USkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsCoolingDown)
	{
		CurrentCooldownTime -= InDeltaTime;

		if (CurrentCooldownTime <= 0.0f)
		{
			EndCooldown();
		}
		else
		{
			float Percent = CurrentCooldownTime / TotalCooldownTime;
			if (CooldownBar)
			{
				CooldownBar->SetPercent(Percent);
			}

			if (CooldownText)
			{
				FString TimeStr = FString::Printf(TEXT("%.1f"), CurrentCooldownTime);
				CooldownText->SetText(FText::FromString(TimeStr));
			}
		}
	}
}

void USkillSlotWidget::StartCooldown(float Duration)
{
	if (Duration <= 0.0f) return;

	TotalCooldownTime = Duration;
	CurrentCooldownTime = Duration;
	bIsCoolingDown = true;

	if (CooldownBar)
	{
		CooldownBar->SetVisibility(ESlateVisibility::Visible);
		CooldownBar->SetPercent(1.0f);
	}
	if (CooldownText)
	{
		CooldownText->SetVisibility(ESlateVisibility::Visible);
	}
}

void USkillSlotWidget::EndCooldown()
{
	bIsCoolingDown = false;
	CurrentCooldownTime = 0.0f;

	if (CooldownBar) CooldownBar->SetVisibility(ESlateVisibility::Hidden);
	if (CooldownText) CooldownText->SetVisibility(ESlateVisibility::Hidden);
}