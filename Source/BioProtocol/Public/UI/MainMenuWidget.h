// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BioProtocol/Session/SessionSubsystem.h"
#include "MainMenuWidget.generated.h"

class UEditableTextBox;
class UButton;
class UScrollBox;
class USessionRowWidget;

UCLASS()
class BIOPROTOCOL_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_FindSession;

	UPROPERTY(meta = (BindWidget))
	UButton* Btn_CreateSession;

	UPROPERTY(meta = (BindWidget))
	UScrollBox* SessionListScrollBox;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> SessionSlotClass;

public:
	void JoinServerAtIndex(int32 Index);

private:
	UFUNCTION()
	void OnFindSessionClicked();

	UFUNCTION()
	void OnCreateSessionClicked();

	UFUNCTION()
	void UpdateSessionList(const TArray<FSessionInfo>& SessionInfos);

	USessionSubsystem* GetSessionSubsystem() const;
};