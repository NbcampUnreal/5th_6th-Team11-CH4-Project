// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractionWidget.generated.h"

class UTextBlock;

UCLASS()
class BIOPROTOCOL_API UInteractionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ShowInteraction(FString Message);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void HideInteraction();

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ActionText;
};
