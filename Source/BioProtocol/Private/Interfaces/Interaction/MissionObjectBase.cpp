// Fill out your copyright notice in the Description page of Project Settings.

#include "Interfaces/Interaction/MissionObjectBase.h"
#include "BioProtocol/Character/DXPlayerCharacter.h"
#include "BioProtocol/Public/Equippable/EquippableItem.h"
#include "BioProtocol/Public/Inventory/InventoryComponent.h"
#include "BioProtocol/Public/Items/ItemBase.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Character/StaffCharacter.h"

// Sets default values
AMissionObjectBase::AMissionObjectBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	ObjectMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectMesh"));
	ObjectMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ObjectMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SetRootComponent(ObjectMesh);

	RequiredToolID = NAME_None;
	CompletionTime = 5.0f;
	bIsCompleted = false;
	bInProgress = false;
	CurrentWorker = nullptr;
	Progress = 0.0f;

	WorkingParticle = nullptr;
	WorkingSound = nullptr;
	CompletionSound = nullptr;
	ActiveWorkingParticle = nullptr;
	ActiveWorkingSound = nullptr;

	InstanceInteractableData.InteractableType = EInteractableType::MissionObject;
	InstanceInteractableData.Name = FText::FromString("Mission Object");
	InstanceInteractableData.Action = FText::FromString("Repair");
	InstanceInteractableData.InteractionDuration = CompletionTime;
	InstanceInteractableData.RequiredToolID = RequiredToolID;
	InstanceInteractableData.bCanInteract = true;

}

// Called when the game starts or when spawned
void AMissionObjectBase::BeginPlay()
{
	Super::BeginPlay();

	InteractableData = InstanceInteractableData;
	
}

void AMissionObjectBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMissionObjectBase, bIsCompleted);
	DOREPLIFETIME(AMissionObjectBase, bInProgress);
	DOREPLIFETIME(AMissionObjectBase, CurrentWorker);
	DOREPLIFETIME(AMissionObjectBase, Progress);
}

//==========================================
// INTERACTION INTERFACE
//==========================================

void AMissionObjectBase::BeginFocus_Implementation()
{
	if (ObjectMesh)
	{
		ObjectMesh->SetRenderCustomDepth(true);
	}
}

void AMissionObjectBase::EndFocus_Implementation()
{
	if (ObjectMesh)
	{
		ObjectMesh->SetRenderCustomDepth(false);
	}
}

void AMissionObjectBase::BeginInteract_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("[MissionObject] BeginInteract"));
}

void AMissionObjectBase::EndInteract_Implementation()
{
	if (bInProgress)
	{
		CancelWork();
	}
}

void AMissionObjectBase::Interact_Implementation(AStaffCharacter* PlayerCharacter)
{
	if (!PlayerCharacter || !HasAuthority())
	{
		return;
	}

	StartWork(PlayerCharacter);
}

FInteractableData AMissionObjectBase::GetInteractableData_Implementation() const
{
	return InstanceInteractableData;
}

bool AMissionObjectBase::CanInteract_Implementation(AStaffCharacter* PlayerCharacter) const
{
	if (!PlayerCharacter || bIsCompleted || bInProgress)
	{
		return false;
	}

	// 필요한 도구 확인
	if (!RequiredToolID.IsNone())
	{
		return HasRequiredTool(PlayerCharacter);
	}

	return true;
}

//==========================================
// WORK FUNCTIONS
//==========================================

void AMissionObjectBase::StartWork(AStaffCharacter* Worker)
{
	if (!HasAuthority())
	{
		ServerStartWork(Worker);
		return;
	}

	if (!Worker || bIsCompleted || bInProgress)
	{
		return;
	}

	// 도구 확인
	if (!RequiredToolID.IsNone() && !HasRequiredTool(Worker))
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionObject] Player doesn't have required tool: %s"),
			*RequiredToolID.ToString());
		return;
	}

	bInProgress = true;
	CurrentWorker = Worker;
	Progress = 0.0f;

	StartWorkEffects();

	// 진행도 업데이트 타이머 (0.1초마다)
	GetWorldTimerManager().SetTimer(
		WorkTimerHandle,
		[this]()
		{
			UpdateProgress(0.1f);
		},
		0.1f,
		true
	);

//	UE_LOG(LogTemp, Log, TEXT("[MissionObject] Work started by %s"), *Worker->GetName());
}

void AMissionObjectBase::CancelWork()
{
	if (!HasAuthority())
	{
		ServerCancelWork();
		return;
	}

	GetWorldTimerManager().ClearTimer(WorkTimerHandle);
	StopWorkEffects();

	bInProgress = false;
	CurrentWorker = nullptr;
	Progress = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("[MissionObject] Work cancelled"));
}

void AMissionObjectBase::CompleteWork()
{
	if (!HasAuthority())
	{
		ServerCompleteWork();
		return;
	}

	GetWorldTimerManager().ClearTimer(WorkTimerHandle);
	StopWorkEffects();

	bInProgress = false;
	bIsCompleted = true;
	Progress = 1.0f;

	// 완료 사운드
	if (CompletionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CompletionSound, GetActorLocation());
	}

	// 델리게이트
	OnMissionCompleted.Broadcast(this);

	UE_LOG(LogTemp, Log, TEXT("[MissionObject] Work completed!"));
}

void AMissionObjectBase::UpdateProgress(float DeltaTime)
{
	if (!bInProgress || !CurrentWorker)
	{
		CancelWork();
		return;
	}

	// 플레이어가 멀어지면 취소
	float Distance = FVector::Dist(GetActorLocation(), CurrentWorker->GetActorLocation());
	if (Distance > 300.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MissionObject] Player too far, cancelling work"));
		CancelWork();
		return;
	}

	// 진행도 증가
	Progress += DeltaTime / CompletionTime;
	OnProgressChanged.Broadcast(Progress, 1.0f);

	// 완료 확인
	if (Progress >= 1.0f)
	{
		CompleteWork();
	}
}
//==========================================
// EFFECTS
//==========================================

void AMissionObjectBase::StartWorkEffects()
{
	if (WorkingParticle && ObjectMesh && !ActiveWorkingParticle)
	{
		ActiveWorkingParticle = UGameplayStatics::SpawnEmitterAttached(
			WorkingParticle,
			ObjectMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true
		);
	}

	if (WorkingSound && ObjectMesh && !ActiveWorkingSound)
	{
		ActiveWorkingSound = UGameplayStatics::SpawnSoundAttached(
			WorkingSound,
			ObjectMesh,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			false,
			1.0f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			true
		);
	}
}

void AMissionObjectBase::StopWorkEffects()
{
	if (ActiveWorkingParticle)
	{
		ActiveWorkingParticle->DeactivateSystem();
		ActiveWorkingParticle = nullptr;
	}

	if (ActiveWorkingSound)
	{
		ActiveWorkingSound->Stop();
		ActiveWorkingSound = nullptr;
	}
}

bool AMissionObjectBase::HasRequiredTool(AStaffCharacter* Player) const
{
	if (!Player || RequiredToolID.IsNone())
	{
		return true;
	}

	// 현재 장착된 아이템 확인
	AEquippableItem* CurrentItem = Player->GetCurrentEquippedItem();
	if (!CurrentItem || !CurrentItem->ItemReference)
	{
		return false;
	}

	return CurrentItem->ItemReference->ItemID == RequiredToolID;
}

//==========================================
// SERVER RPC
//==========================================

void AMissionObjectBase::ServerStartWork_Implementation(AStaffCharacter* Worker)
{
	StartWork(Worker);
}

void AMissionObjectBase::ServerCancelWork_Implementation()
{
	CancelWork();
}

void AMissionObjectBase::ServerCompleteWork_Implementation()
{
	CompleteWork();
}

void AMissionObjectBase::OnRep_IsCompleted()
{
	if (bIsCompleted)
	{
		UE_LOG(LogTemp, Log, TEXT("[MissionObject Client] Mission completed"));

		// 1. 동적 머티리얼 인스턴스 생성 및 색상 변경
		if (ObjectMesh)
		{
			// 모든 머티리얼 슬롯에 동적 머티리얼 적용
			for (int32 i = 0; i < ObjectMesh->GetNumMaterials(); i++)
			{
				UMaterialInstanceDynamic* DynMat = ObjectMesh->CreateDynamicMaterialInstance(i);
				if (DynMat)
				{
					// 초록색 발광 효과 (완료 표시)
					DynMat->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor(0.0f, 1.0f, 0.0f, 1.0f));
					DynMat->SetScalarParameterValue(FName("EmissiveStrength"), 5.0f);

					// 베이스 컬러 변경 (선택사항)
					DynMat->SetVectorParameterValue(FName("BaseColor"), FLinearColor(0.2f, 0.8f, 0.2f, 1.0f));

					// 메탈릭/러프니스 조정으로 더욱 반짝이게
					DynMat->SetScalarParameterValue(FName("Metallic"), 0.8f);
					DynMat->SetScalarParameterValue(FName("Roughness"), 0.3f);
				}
			}

			// 2. 커스텀 뎁스 비활성화 (더 이상 상호작용 불가)
			ObjectMesh->SetRenderCustomDepth(false);
		}

		// 3. 완료 파티클 이펙트 재생
		if (CompletionParticle)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				CompletionParticle,
				GetActorLocation(),
				FRotator::ZeroRotator,
				FVector(1.5f), // 크기
				true, // Auto Destroy
				EPSCPoolMethod::AutoRelease
			);
		}

		// 4. 완료 사운드 재생
		if (CompletionSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				CompletionSound,
				GetActorLocation(),
				1.0f, // Volume
				1.0f  // Pitch
			);
		}

		// 5. 메시에 펄스 애니메이션 효과 (선택사항)
		if (ObjectMesh)
		{
			// 타임라인을 사용하거나 간단하게 타이머로 구현
			FTimerHandle PulseTimerHandle;
			GetWorldTimerManager().SetTimer(
				PulseTimerHandle,
				[this]()
				{
					if (ObjectMesh)
					{
						// 스케일 펄스 효과
						FVector CurrentScale = ObjectMesh->GetRelativeScale3D();
						float PulseAmount = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.0f) * 0.05f + 1.0f;
						ObjectMesh->SetRelativeScale3D(FVector(PulseAmount));
					}
				},
				0.05f,
				true,
				0.0f
			);

			// 3초 후 펄스 중지
			GetWorldTimerManager().SetTimer(
				PulseTimerHandle,
				[this]()
				{
					if (ObjectMesh)
					{
						ObjectMesh->SetRelativeScale3D(FVector(1.0f));
					}
				},
				3.0f,
				false
			);
		}

		// 6. UI 알림 (델리게이트 사용)
		OnMissionCompleted.Broadcast(this);

		// 7. 진동 피드백 (컨트롤러)
		if (CurrentWorker && CurrentWorker->IsLocallyControlled())
		{
			APlayerController* PC = Cast<APlayerController>(CurrentWorker->GetController());
			if (PC)
			{
				// 간단한 진동 피드백
				PC->PlayDynamicForceFeedback(
					0.5f,  // Intensity
					0.3f,  // Duration
					true,  // Large motors
					true,  // Small motors
					true,  // Left
					true   // Right
				);
			}
		}
	}
}