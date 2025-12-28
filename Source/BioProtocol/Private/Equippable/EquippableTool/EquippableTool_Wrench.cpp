// Fill out your copyright notice in the Description page of Project Settings.


#include "Equippable/EquippableTool/EquippableTool_Wrench.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AEquippableTool_Wrench::AEquippableTool_Wrench()
{
	WrenchUseSound = nullptr;
	WrenchUseParticle = nullptr;
	WrenchUseMontage = nullptr;
}

void AEquippableTool_Wrench::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("[Wrench] Wrench initialized"));
}

void AEquippableTool_Wrench::Use()
{
	Super::Use();  // 부모의 Use 호출 (bIsInUse = true)

	PlayUseAnimation();

	// 렌치 사용 사운드 재생
	if (WrenchUseSound && OwningCharacter)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			WrenchUseSound,
			OwningCharacter->GetActorLocation()
		);
	}

	// 렌치 사용 파티클 재생
	if (WrenchUseParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			WrenchUseParticle,
			GetActorLocation(),
			GetActorRotation()
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[Wrench] Wrench used successfully"));
}

void AEquippableTool_Wrench::StopUsing()
{
	Super::StopUsing();  // 부모의 StopUsing 호출 (bIsInUse = false)

	UE_LOG(LogTemp, Log, TEXT("[Wrench] Wrench usage stopped"));
}

bool AEquippableTool_Wrench::CanUse() const
{
	// 렌치는 특별한 제약 없이 부모 조건만 체크
	return Super::CanUse();
}

void AEquippableTool_Wrench::PlayUseAnimation()
{
	if (!OwningCharacter || !WrenchUseMontage)
		return;

	OwningCharacter->PlayToolUseMontage(WrenchUseMontage);
}