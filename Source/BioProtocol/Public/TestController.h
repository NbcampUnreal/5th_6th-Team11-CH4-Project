// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TestController.generated.h"

/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API ATestController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SessionWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> SessionWidgetInstance;


public:
	UFUNCTION(BlueprintCallable)
	void HostGame();

	UFUNCTION(BlueprintCallable)
	void FindGames();

	UFUNCTION(BlueprintCallable)
	void JoinGame();
};
