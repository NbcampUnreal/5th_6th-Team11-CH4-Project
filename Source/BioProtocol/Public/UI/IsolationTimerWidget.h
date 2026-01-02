// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IsolationTimerWidget.generated.h"

class UTextBlock;

UCLASS()
class BIOPROTOCOL_API UIsolationTimerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* TimeText;

public:
	void UpdateTime(float TimeRemaining);
};