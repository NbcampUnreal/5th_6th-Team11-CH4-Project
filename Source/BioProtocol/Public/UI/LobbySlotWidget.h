// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbySlotWidget.generated.h"

class UTextBlock;
class UCheckBox;
class ALobbyPlayerState;

UCLASS()
class BIOPROTOCOL_API ULobbySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateSlot(ALobbyPlayerState* PlayerState);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	UCheckBox* ReadyCheckBox;

	TWeakObjectPtr<ALobbyPlayerState> TargetPlayerState;

	UFUNCTION()
	void UpdateReadyState();
};