
#include "TESTPlayerState.h"
#include "Net/UnrealNetwork.h"

void ATESTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATESTPlayerState, bisReady);
}
