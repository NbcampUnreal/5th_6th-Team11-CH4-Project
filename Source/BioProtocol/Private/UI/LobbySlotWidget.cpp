// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Session/LobbyPlayerState.h"

void ULobbySlotWidget::UpdateSlot(ALobbyPlayerState* PlayerState)
{
	if (!PlayerState) return;

	TargetPlayerState = PlayerState;

	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(PlayerState->GetPlayerName()));
	}

	if (!PlayerState->OnStatusChanged.IsAlreadyBound(this, &ULobbySlotWidget::UpdateReadyState))
	{
		PlayerState->OnStatusChanged.AddDynamic(this, &ULobbySlotWidget::UpdateReadyState);
	}

	UpdateReadyState();
}

void ULobbySlotWidget::UpdateReadyState()
{
	if (TargetPlayerState.IsValid())
	{
		if (ReadyCheckBox)
		{
			ReadyCheckBox->SetIsChecked(TargetPlayerState->bIsReady);
			ReadyCheckBox->SetIsEnabled(false);
		}
	}
}