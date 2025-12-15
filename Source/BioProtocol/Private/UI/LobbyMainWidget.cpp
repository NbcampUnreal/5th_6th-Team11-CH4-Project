// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyMainWidget.h"
#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/LobbySlotWidget.h"
#include "Session/LobbyPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

void ULobbyMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ToggleReadyButton)
	{
		ToggleReadyButton->OnClicked.AddDynamic(this, &ULobbyMainWidget::OnReadyClicked);
	}

	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &ULobbyMainWidget::OnExitClicked);
	}
}

void ULobbyMainWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsBound)
	{
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			ALobbyPlayerState* MyPS = PC->GetPlayerState<ALobbyPlayerState>();
			if (MyPS)
			{
				LocalPlayerState = MyPS;

				MyPS->OnStatusChanged.AddDynamic(this, &ULobbyMainWidget::UpdateButtonText);

				UpdateButtonText();

				bIsBound = true;
			}
		}
	}

	AGameStateBase* GameState = UGameplayStatics::GetGameState(GetWorld());
	if (GameState)
	{
		TArray<APlayerState*> PlayerArray = GameState->PlayerArray;

		if (PlayerArray.Num() != LastPlayerCount)
		{
			RefreshPlayerList();
			LastPlayerCount = PlayerArray.Num();
		}
		else
		{

		}
	}
}

void ULobbyMainWidget::RefreshPlayerList()
{
	if (!PlayerListScrollBox || !SlotWidgetClass) return;

	PlayerListScrollBox->ClearChildren();

	AGameStateBase* GameState = UGameplayStatics::GetGameState(GetWorld());
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			ALobbyPlayerState* BioPS = Cast<ALobbyPlayerState>(PS);
			if (BioPS)
			{
				ULobbySlotWidget* NewSlotWidget = CreateWidget<ULobbySlotWidget>(this, SlotWidgetClass);
				if (NewSlotWidget)
				{
					NewSlotWidget->UpdateSlot(BioPS);
					PlayerListScrollBox->AddChild(NewSlotWidget);
				}
			}
		}
	}
}

void ULobbyMainWidget::OnReadyClicked()
{
	if (LocalPlayerState.IsValid())
	{
		LocalPlayerState->ServerToggleReady();
	}
}

void ULobbyMainWidget::UpdateButtonText()
{
	if (LocalPlayerState.IsValid() && ReadyButtonText)
	{
		bool bReady = LocalPlayerState->bIsReady;

		if (bReady)
		{
			ReadyButtonText->SetText(FText::FromString(TEXT("준비 취소")));
			ToggleReadyButton->SetBackgroundColor(FLinearColor::Red);
		}
		else
		{
			ReadyButtonText->SetText(FText::FromString(TEXT("준비")));
			ToggleReadyButton->SetBackgroundColor(FLinearColor::Green);
		}
	}
}

void ULobbyMainWidget::OnExitClicked()
{
	FName MainMenuLevelName = FName(TEXT("MainMenu"));

	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}