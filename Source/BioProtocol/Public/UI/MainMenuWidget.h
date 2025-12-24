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

	//UPROPERTY(meta = (BindWidget))
	//UEditableTextBox* NicknameInput;

	UPROPERTY(meta = (BindWidget))
	UButton* FindSessionsButton;

	UPROPERTY(meta = (BindWidget))
	UScrollBox* SessionListScrollBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TSubclassOf<USessionRowWidget> SessionRowWidgetClass;

public:
	void JoinServerAtIndex(int32 Index);

private:
	UFUNCTION()
	void OnFindSessionsClicked();

	UFUNCTION()
	void OnSessionSearchCompleted(const TArray<FSessionInfo>& Sessions);

	UFUNCTION()
	void OnJoinSessionCompleted(EJoinResultBP Result);

	//void ApplyNickname();
	USessionSubsystem* GetSessionSubsystem() const;
};