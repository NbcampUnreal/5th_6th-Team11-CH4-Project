// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/LobbyViewModel.h"
#include "LobbySlotWidget.generated.h"

class UTextBlock;
class UCheckBox;
class ABioPlayerState;
class UButton;
class UImage;
class UGridPanel;

UCLASS()
class BIOPROTOCOL_API ULobbySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateSlot(ABioPlayerState* PlayerState);
	void Setup(ABioPlayerState* PlayerState, int32 SlotIndex);
	virtual void NativeDestruct() override;
protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	UImage* CharacterImage;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* ReadyCheckBox;

	UPROPERTY(EditAnywhere, Category = "Config")
	TSubclassOf<ALobbyViewModel> ViewModelActorClass;

	UPROPERTY(EditAnywhere, Category = "Config")
	UMaterialInterface* ViewModelMaterialBase;

	UPROPERTY(meta = (BindWidget))
	UGridPanel* ColorButtonContainer;

	TWeakObjectPtr<ABioPlayerState> TargetPlayerState;

	UFUNCTION()
	void UpdatePlayerInfo();

private:
	UPROPERTY()
	ALobbyViewModel* MyViewModel;

	UPROPERTY()
	UMaterialInstanceDynamic* UI_MaterialInstance;

	void UpdateCharacterMaterials(UMaterialInterface* M1, UMaterialInterface* M2);

	void ApplyMaterialByIndex(int32 Index);

	UFUNCTION()
	void UpdateColorVisuals(int32 NewColorIndex); // 색상만 갱신

	UFUNCTION()
	void UpdateReadyVisuals(bool bIsReady); // 준비 상태 갱신

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Red;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Yellow;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Blue;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Green;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Brown;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Pink;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_White;
	UPROPERTY(meta = (BindWidget))
	UButton* Button_Cyan;

	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Red_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Red_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Yellow_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Yellow_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Blue_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Blue_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Green_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Green_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Brown_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Brown_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Pink_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Pink_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_White_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_White_02;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Cyan_01;
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInterface* Mat_Cyan_02;

private:
	UFUNCTION()
	void OnRedClicked();
	UFUNCTION()
	void OnYellowClicked();
	UFUNCTION()
	void OnBlueClicked();
	UFUNCTION()
	void OnGreenClicked();
	UFUNCTION()
	void OnBrownClicked();
	UFUNCTION()
	void OnPinkClicked();
	UFUNCTION()
	void OnWhiteClicked();
	UFUNCTION()
	void OnCyanClicked();
};