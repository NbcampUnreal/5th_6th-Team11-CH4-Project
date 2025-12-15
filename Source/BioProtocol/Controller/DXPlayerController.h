// DXPlayerController.h

#pragma once

#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "DXPlayerController.generated.h"

/**
 *
 */
UCLASS()
class BIOPROTOCOL_API ADXPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputMappingContext* DefaultIMC;

};
