// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyGameModeBase.generated.h"

class AThirdSpectatorPawn;
/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API AMyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	virtual void OnPlayerKilled(AController* VictimController);

	UPROPERTY(EditDefaultsOnly, Category = "Spectator")
	TSubclassOf<AThirdSpectatorPawn> SpectatorPawnClass;
};
