// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/BioPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "MyTestGameInstance.h"

ABioPlayerState::ABioPlayerState()
{
	GameRole = EBioPlayerRole::Staff;
	ColorIndex = -1;
	bIsReady = false;
}

void ABioPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABioPlayerState, GameRole);
	DOREPLIFETIME(ABioPlayerState, ColorIndex);
	DOREPLIFETIME(ABioPlayerState, bIsReady);
	DOREPLIFETIME(ABioPlayerState, EOSPlayerName);
}

void ABioPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TryInitEOSPlayerName();
	}
}

void ABioPlayerState::OnRep_GameRole()
{
	OnRoleChanged.Broadcast(GameRole);
	UE_LOG(LogTemp, Log, TEXT("OnRep_GameRole: Role received as %d"), (int32)GameRole);
}

void ABioPlayerState::OnRep_EOSPlayerName()
{
	// EOS 플레이어 이름 변경 시 필요한 로직 추가
}

void ABioPlayerState::Server_RequestRoleChange_Implementation(EBioPlayerRole NewRole)
{
	if (!HasAuthority())
		return;

	GameRole = NewRole;
	ForceNetUpdate();

	// EOS 플레이어 이름이 있으면 게임 인스턴스에 저장
	if (!EOSPlayerName.IsEmpty())
	{
		UMyTestGameInstance* GI = Cast<UMyTestGameInstance>(GetWorld()->GetGameInstance());
		if (GI)
		{
			GI->SavePlayerRole(EOSPlayerName, NewRole);
		}
	}
}

void ABioPlayerState::Server_SetGameRole(EBioPlayerRole NewRole)
{
	if (!HasAuthority())
		return;

	GameRole = NewRole;
	ForceNetUpdate();
}

void ABioPlayerState::Server_SetEOSPlayerName(const FString& InEOSPlayerName)
{
	EOSPlayerName = InEOSPlayerName;
	ForceNetUpdate();
}

void ABioPlayerState::TryInitEOSPlayerName()
{
	// 이미 이름이 설정되어 있으면 역할 복원
	if (!EOSPlayerName.IsEmpty())
	{
		//RestoreRoleFromGameInstance();
		return;
	}

	// UniqueId가 유효하지 않으면 재시도
	if (!GetUniqueId().IsValid())
	{
		GetWorldTimerManager().SetTimer(EOSNameInitTimer, this,
			&ABioPlayerState::TryInitEOSPlayerName, 0.2f, false);
		return;
	}

	// UniqueId에서 EOS 플레이어 이름 추출
	FString Raw = GetUniqueId()->ToString();
	FString Left, Right;
	if (Raw.Split(TEXT("|"), &Left, &Right))
	{
		EOSPlayerName = Right;
	}
	else
	{
		EOSPlayerName = Raw;
	}

	ForceNetUpdate();
}

void ABioPlayerState::RestoreRoleFromGameInstance()
{
	// EOS 플레이어 이름이 없으면 재시도
	if (EOSPlayerName.IsEmpty())
	{
		FTimerHandle RetryTimer;
		GetWorldTimerManager().SetTimer(RetryTimer, this,
			&ABioPlayerState::RestoreRoleFromGameInstance, 0.2f, false);
		return;
	}

	UMyTestGameInstance* GI = Cast<UMyTestGameInstance>(GetWorld()->GetGameInstance());
	if (!GI)
	{
		return;
	}
}