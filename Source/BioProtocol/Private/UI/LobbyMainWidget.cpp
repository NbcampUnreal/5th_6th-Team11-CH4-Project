// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyMainWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/LobbySlotWidget.h"
#include "Game/BioPlayerState.h"
#include "BioProtocol/Session/SessionSubsystem.h"
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
	if (StartGameButton)
	{
		StartGameButton->OnClicked.AddDynamic(this, &ULobbyMainWidget::OnStartGameClicked);
		StartGameButton->SetIsEnabled(false);
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
			ABioPlayerState* MyPS = PC->GetPlayerState<ABioPlayerState>();
			if (MyPS)
			{
				LocalPlayerState = MyPS;
				UpdateButtonText();
				bIsBound = true;
			}
		}
	}

	if (LocalPlayerState.IsValid())
	{
		UpdateButtonText();
	}

	AGameStateBase* GameState = UGameplayStatics::GetGameState(GetWorld());
	if (GameState)
	{
		if (GameState->PlayerArray.Num() != LastPlayerCount)
		{
			RefreshPlayerList();
			LastPlayerCount = GameState->PlayerArray.Num();
		}
		if (StartGameButton && StartGameButton->GetVisibility() == ESlateVisibility::Visible)
		{
			bool bAllReady = true;

			if (GameState->PlayerArray.Num() == 0)
			{
				bAllReady = false;
			}
			else
			{
				for (APlayerState* PS : GameState->PlayerArray)
				{
					if (ABioPlayerState* BioPS = Cast<ABioPlayerState>(PS))
					{
						if (!BioPS->bIsReady)
						{
							bAllReady = false;
							break;
						}
					}
				}
			}
			if (StartGameButton->GetIsEnabled() != bAllReady)
			{
				StartGameButton->SetIsEnabled(bAllReady);
				StartGameButton->SetRenderOpacity(bAllReady ? 1.0f : 0.5f);
			}
		}
	}
}

void ULobbyMainWidget::RefreshPlayerList()
{

	if (!PlayerListBox || !SlotWidgetClass) return;

	PlayerListBox->ClearChildren();

	AGameStateBase* GameState = UGameplayStatics::GetGameState(GetWorld());
	if (GameState)
	{
		TArray<APlayerState*> PlayerArray = GameState->PlayerArray;

		for (int32 i = 0; i < PlayerArray.Num(); ++i)
		{
			ABioPlayerState* BioPS = Cast<ABioPlayerState>(PlayerArray[i]);
			if (BioPS)
			{
				AddPlayerSlot(BioPS, i);
			}
		}
	}
}

void ULobbyMainWidget::AddPlayerSlot(ABioPlayerState* PlayerState, int32 SlotIndex)
{
	if (!SlotWidgetClass || !PlayerListBox) return;

	ULobbySlotWidget* NewSlotWidget = CreateWidget<ULobbySlotWidget>(this, SlotWidgetClass);
	if (NewSlotWidget)
	{
		NewSlotWidget->Setup(PlayerState, SlotIndex);
		PlayerListBox->AddChild(NewSlotWidget);
	}
}

void ULobbyMainWidget::OnReadyClicked()
{
	if (LocalPlayerState.IsValid())
	{
		LocalPlayerState->ServerToggleReady();
	}
}

void ULobbyMainWidget::OnStartGameClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USessionSubsystem* SessionSub = GI->GetSubsystem<USessionSubsystem>())
		{
			SessionSub->StartGameSession();
		}
	}
}

void ULobbyMainWidget::UpdateButtonText()
{
	if (LocalPlayerState.IsValid() && ReadyButtonText && ToggleReadyButton)
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