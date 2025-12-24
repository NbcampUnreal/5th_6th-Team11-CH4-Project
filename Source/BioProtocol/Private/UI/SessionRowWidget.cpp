// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SessionRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "UI/MainMenuWidget.h" 

void USessionRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &USessionRowWidget::OnJoinButtonClicked);
	}
}

void USessionRowWidget::Setup(const FSessionInfo& InInfo, int32 InIndex)
{
	SessionIndex = InIndex;

	if (SessionNameText)
	{
		SessionNameText->SetText(FText::FromString(InInfo.SessionName));
	}

	if (ConnectionCountText)
	{
		FString PlayerCountStr = FString::Printf(TEXT("%d / %d"), InInfo.CurrentPlayers, InInfo.MaxPlayers);
		ConnectionCountText->SetText(FText::FromString(PlayerCountStr));
	}

	if (PingText)
	{
		FString PingStr = FString::Printf(TEXT("%d ms"), InInfo.PingInMs);
		PingText->SetText(FText::FromString(PingStr));
	}
}

void USessionRowWidget::SetParentMenu(UMainMenuWidget* InParentWidget)
{
	ParentMenu = InParentWidget;
}

void USessionRowWidget::OnJoinButtonClicked()
{
	if (ParentMenu)
	{
		ParentMenu->JoinServerAtIndex(SessionIndex);
	}
}
