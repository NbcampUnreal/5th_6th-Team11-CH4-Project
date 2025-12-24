// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BioGameMode.generated.h"

class ABioGameState;
class ABioPlayerState;

UCLASS()
class BIOPROTOCOL_API ABioGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ABioGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	void SendPlayerToJail(AController* PlayerToJail);

protected:
	FVector JailLocation;

	UPROPERTY(EditDefaultsOnly, Category = "Bio|GameRule")
	float DayDuration = 240.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Bio|GameRule")
	int32 NightDuration = 45;

	float TimeCounter = 0.0f;
	void AssignRoles();
	void UpdateJailTimers(float DeltaTime);
	void CheckWinConditions();

	UPROPERTY()
	ABioGameState* BioGameState;
};