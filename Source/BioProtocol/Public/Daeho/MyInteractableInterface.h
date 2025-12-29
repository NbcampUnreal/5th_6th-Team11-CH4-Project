#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MyInteractableInterface.generated.h"

UINTERFACE(MinimalAPI)
class UMyInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class BIOPROTOCOL_API IMyInteractableInterface
{
	GENERATED_BODY()

public:
	// "BlueprintNativeEvent"를 추가해야 Execute_Interact가 생성됩니다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InstigatorPawn);
};
