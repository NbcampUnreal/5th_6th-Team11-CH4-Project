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
	// "BlueprintNativeEvent"瑜?異붽??댁빞 Execute_Interact媛 ?앹꽦?⑸땲??
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APawn* InstigatorPawn);
};
