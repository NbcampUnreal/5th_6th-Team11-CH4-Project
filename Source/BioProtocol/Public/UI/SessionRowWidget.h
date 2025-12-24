// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BioProtocol/Session/SessionSubsystem.h"
#include "SessionRowWidget.generated.h"

class UTextBlock;
class UButton;
class UMainMenuWidget;

UCLASS()
class BIOPROTOCOL_API USessionRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FSessionInfo& InInfo, int32 InIndex);

	void SetParentMenu(UMainMenuWidget* InParentWidget);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnJoinButtonClicked();

private:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* SessionNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ConnectionCountText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PingText;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	int32 SessionIndex;

	UPROPERTY()
	UMainMenuWidget* ParentMenu;
};