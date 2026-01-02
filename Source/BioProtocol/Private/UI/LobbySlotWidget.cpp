// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbySlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Game/BioPlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/GridPanel.h"

void ULobbySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Red)
	{
		Button_Red->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnRedClicked);
	}
	if (Button_Yellow)
	{
		Button_Yellow->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnYellowClicked);
	}
	if (Button_Blue)
	{
		Button_Blue->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnBlueClicked);
	}
	if (Button_Green)
	{
		Button_Green->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnGreenClicked);
	}
	if (Button_Brown)
	{
		Button_Brown->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnBrownClicked);
	}
	if (Button_Pink)
	{
		Button_Pink->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnPinkClicked);
	}
	if (Button_White)
	{
		Button_White->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnWhiteClicked);
	}
	if (Button_Cyan)
	{
		Button_Cyan->OnClicked.AddDynamic(this, &ULobbySlotWidget::OnCyanClicked);
	}
}

void ULobbySlotWidget::NativeDestruct()
{
	Super::NativeDestruct();
	if (MyViewModel)
	{
		MyViewModel->Destroy();
		MyViewModel = nullptr;
	}
}

void ULobbySlotWidget::Setup(ABioPlayerState* PlayerState, int32 SlotIndex)
{
	if (!PlayerState) return;

	TargetPlayerState = PlayerState;

	FString NameToDisplay = PlayerState->GetPlayerName();
	if (PlayerNameText)
	{
		PlayerNameText->SetText(FText::FromString(NameToDisplay));
	}

	if (ColorButtonContainer)
	{
		ColorButtonContainer->SetVisibility(ESlateVisibility::Collapsed);

		APlayerController* PC = GetOwningPlayer();
		if (PC && PC->PlayerState == PlayerState)
		{
			ColorButtonContainer->SetVisibility(ESlateVisibility::Visible);
		}
	}

	UWorld* World = GetWorld();
	if (World && ViewModelActorClass)
	{
		FVector SpawnLoc(0, SlotIndex * 500.0f, -10000.0f);
		FRotator SpawnRot = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (MyViewModel) MyViewModel->Destroy();
		MyViewModel = World->SpawnActor<ALobbyViewModel>(ViewModelActorClass, SpawnLoc, SpawnRot, SpawnParams);

		if (MyViewModel && CharacterImage)
		{
			UTextureRenderTarget2D* RT = MyViewModel->InitShowroom();
			if (RT && ViewModelMaterialBase)
			{
				UMaterialInstanceDynamic* DMI = UMaterialInstanceDynamic::Create(ViewModelMaterialBase, this);

				if (DMI)
				{
					DMI->SetTextureParameterValue(FName("ViewTexture"), RT);
					CharacterImage->SetBrushFromMaterial(DMI);
				}
			}
		}
	}

	if (TargetPlayerState.IsValid())
	{
		TargetPlayerState->OnColorChanged.RemoveDynamic(this, &ULobbySlotWidget::UpdateColorVisuals);
		TargetPlayerState->OnReadyStatusChanged.RemoveDynamic(this, &ULobbySlotWidget::UpdateReadyVisuals);
		TargetPlayerState->OnColorChanged.AddDynamic(this, &ULobbySlotWidget::UpdateColorVisuals);
		TargetPlayerState->OnReadyStatusChanged.AddDynamic(this, &ULobbySlotWidget::UpdateReadyVisuals);

		UpdatePlayerInfo();
		UpdateColorVisuals(TargetPlayerState->ColorIndex);
		UpdateReadyVisuals(TargetPlayerState->bIsReady);
	}
}

void ULobbySlotWidget::UpdateSlot(ABioPlayerState* PlayerState)
{
	if (!PlayerState) return;

	if (TargetPlayerState.Get() != PlayerState)
	{
		TargetPlayerState = PlayerState;

		if (PlayerNameText)
		{
			PlayerNameText->SetText(FText::FromString(PlayerState->GetPlayerName()));
		}
		if (TargetPlayerState.IsValid())
		{
			TargetPlayerState->OnColorChanged.RemoveDynamic(this, &ULobbySlotWidget::UpdateColorVisuals);
			TargetPlayerState->OnReadyStatusChanged.RemoveDynamic(this, &ULobbySlotWidget::UpdateReadyVisuals);

			TargetPlayerState->OnColorChanged.AddDynamic(this, &ULobbySlotWidget::UpdateColorVisuals);
			TargetPlayerState->OnReadyStatusChanged.AddDynamic(this, &ULobbySlotWidget::UpdateReadyVisuals);
		}
		UpdatePlayerInfo();
	}
}

void ULobbySlotWidget::UpdatePlayerInfo()
{
	if (!TargetPlayerState.IsValid()) return;

	UpdateReadyVisuals(TargetPlayerState->bIsReady);
	UpdateColorVisuals(TargetPlayerState->ColorIndex);
}

void ULobbySlotWidget::UpdateReadyVisuals(bool bIsReady)
{
	if (ReadyCheckBox)
	{
		ReadyCheckBox->SetIsChecked(bIsReady);
	}
}

void ULobbySlotWidget::UpdateColorVisuals(int32 NewColorIndex)
{
	ApplyMaterialByIndex(NewColorIndex);
}

void ULobbySlotWidget::ApplyMaterialByIndex(int32 Index)
{
	switch (Index)
	{
	case 0: UpdateCharacterMaterials(Mat_Red_01, Mat_Red_02); break;
	case 1: UpdateCharacterMaterials(Mat_Yellow_01, Mat_Yellow_02); break;
	case 2: UpdateCharacterMaterials(Mat_Blue_01, Mat_Blue_02); break;
	case 3: UpdateCharacterMaterials(Mat_Green_01, Mat_Green_02); break;
	case 4: UpdateCharacterMaterials(Mat_Brown_01, Mat_Brown_02); break;
	case 5: UpdateCharacterMaterials(Mat_Pink_01, Mat_Pink_02); break;
	case 6: UpdateCharacterMaterials(Mat_White_01, Mat_White_02); break;
	case 7: UpdateCharacterMaterials(Mat_Cyan_01, Mat_Cyan_02); break;
	default: UpdateCharacterMaterials(Mat_Red_01, Mat_Red_02); break;
	}
}

void ULobbySlotWidget::UpdateCharacterMaterials(UMaterialInterface* M1, UMaterialInterface* M2)
{
	if (MyViewModel)
	{
		MyViewModel->SetCharacterMaterial(M1, M2);
	}
}

void ULobbySlotWidget::OnRedClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(0);
}

void ULobbySlotWidget::OnYellowClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(1);
}

void ULobbySlotWidget::OnBlueClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(2);
}

void ULobbySlotWidget::OnGreenClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(3);
}

void ULobbySlotWidget::OnBrownClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(4);
}

void ULobbySlotWidget::OnPinkClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(5);
}

void ULobbySlotWidget::OnWhiteClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(6);
}

void ULobbySlotWidget::OnCyanClicked()
{
	if (TargetPlayerState.IsValid()) TargetPlayerState->ServerSetColorIndex(7);
}