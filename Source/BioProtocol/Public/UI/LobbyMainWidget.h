// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyMainWidget.generated.h"

class UScrollBox;
class UButton;
class UTextBlock;
class ULobbySlotWidget;
class ALobbyPlayerState;

UCLASS()
class BIOPROTOCOL_API ULobbyMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* PlayerListScrollBox;
	UPROPERTY(meta = (BindWidget))
	UButton* ToggleReadyButton;
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ReadyButtonText;
	UPROPERTY(meta = (BindWidget))
	UButton* ExitButton;

	UPROPERTY(EditDefaultsOnly, Category = "Lobby")
	TSubclassOf<ULobbySlotWidget> SlotWidgetClass;

	TWeakObjectPtr<ALobbyPlayerState> LocalPlayerState;

	int32 LastPlayerCount = 0;

	bool bIsBound = false;

	UFUNCTION()
	void OnReadyClicked();
	UFUNCTION()
	void UpdateButtonText();
	UFUNCTION()
	void OnExitClicked();
	void RefreshPlayerList();

};