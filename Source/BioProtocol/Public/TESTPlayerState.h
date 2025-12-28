// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TESTPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API ATESTPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:


	UPROPERTY(BlueprintReadWrite, Replicated)
	bool bisReady = false;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
