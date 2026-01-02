#include "MyGameStateBase.h"
#include "Net/UnrealNetwork.h"

AMyGameStateBase::AMyGameStateBase()
{
	TotalTasks = 0;
	CompletedTasks = 0;
}

void AMyGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 변수들이 네트워크를 통해 복제되도록 등록
	DOREPLIFETIME(AMyGameStateBase, TotalTasks);
	DOREPLIFETIME(AMyGameStateBase, CompletedTasks);
}

void AMyGameStateBase::AddCompletedTask()
{
	// 서버에서만 실행되어야 함
	if (HasAuthority())
	{
		CompletedTasks++;

		// 디버그 로그
		UE_LOG(LogTemp, Warning, TEXT("Task Progress: %d / %d"), CompletedTasks, TotalTasks);
	}
}

float AMyGameStateBase::GetTaskProgress() const
{
	if (TotalTasks <= 0) return 0.0f;
	return (float)CompletedTasks / (float)TotalTasks;
}



