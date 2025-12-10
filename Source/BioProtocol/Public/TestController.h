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
	TSubclassOf<UUserWidget> CreateSessionWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> CreateSessionWidgetInstance;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> JoinSessionWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> JoinSessionWidgetInstance;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> FindSessionWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> FindSessionWidgetInstance;

public:
	UFUNCTION(BlueprintCallable)
	void HostGame();

	UFUNCTION(BlueprintCallable)
	void FindGames();

	UFUNCTION(BlueprintCallable)
	void JoinFirstGame();
};
