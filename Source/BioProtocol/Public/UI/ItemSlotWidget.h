// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class BIOPROTOCOL_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ItemSlot")
	void UpdateSlot(UTexture2D* Icon);

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon;

};