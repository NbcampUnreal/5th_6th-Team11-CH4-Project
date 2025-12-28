// Fill out your copyright notice in the Description page of Project Settings.


#include "Equippable/EquippableUtility/EquippableUtility_SignalJammer.h"
#include "Components/SphereComponent.h"
#include "BioProtocol/Public/Character/StaffCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "AIController.h" 

AEquippableUtility_SignalJammer::AEquippableUtility_SignalJammer()
{
	PrimaryActorTick.bCanEverTick = true;

	bConsumable = true;
	MaxUses = 1;
	Cooldown = 0.0f;

	Duration = 10.0f;     // 10초 지속
	EffectRadius = 1000.0f;  // 10미터
	bIsActive = false;
	ActivationTime = 0.0f;

	// Sphere Collision 생성
	EffectSphere = CreateDefaultSubobject<USphereComponent>(TEXT("EffectSphere"));
	EffectSphere->SetupAttachment(RootComponent);
	EffectSphere->SetSphereRadius(EffectRadius);
	EffectSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 기본은 비활성
	EffectSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	EffectSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	JammerParticle = nullptr;
	ActiveJammerParticle = nullptr;
}

void AEquippableUtility_SignalJammer::BeginPlay()
{
	Super::BeginPlay();

	// Overlap 이벤트 바인딩
	if (EffectSphere)
	{
		EffectSphere->OnComponentBeginOverlap.AddDynamic(
			this,
			&AEquippableUtility_SignalJammer::OnEffectSphereBeginOverlap
		);
		EffectSphere->OnComponentEndOverlap.AddDynamic(
			this,
			&AEquippableUtility_SignalJammer::OnEffectSphereEndOverlap
		);
	}
}

void AEquippableUtility_SignalJammer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 작동 중이면 남은 시간 체크
	if (bIsActive && HasAuthority())
	{
		float ElapsedTime = GetWorld()->GetTimeSeconds() - ActivationTime;
		if (ElapsedTime >= Duration)
		{
			StopJammer();
		}
	}
}

void AEquippableUtility_SignalJammer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEquippableUtility_SignalJammer, bIsActive);
}

//==========================================
// JAMMER FUNCTIONS
//==========================================

void AEquippableUtility_SignalJammer::ProcessUtilityEffect()
{
	if (!HasAuthority())
	{
		return;
	}

	// 교란기 설치 (바닥에 떨어뜨리기)
	if (OwningCharacter)
	{
		FVector DropLocation = OwningCharacter->GetActorLocation() +
			(OwningCharacter->GetActorForwardVector() * 100.0f);
		SetActorLocation(DropLocation);

		// 캐릭터에서 분리
		Detach();
	}

	// 활성화
	bIsActive = true;
	ActivationTime = GetWorld()->GetTimeSeconds();

	// Collision 활성화
	if (EffectSphere)
	{
		EffectSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	// 이펙트 시작
	StartJammerEffects();

	// 10초 후 자동 종료
	GetWorldTimerManager().SetTimer(
		JammerTimerHandle,
		this,
		&AEquippableUtility_SignalJammer::StopJammer,
		Duration,
		false
	);

	// 0.1초마다 디버프 적용
	GetWorldTimerManager().SetTimer(
		TickTimerHandle,
		this,
		&AEquippableUtility_SignalJammer::ApplyJammerEffect,
		0.1f,
		true
	);

	UE_LOG(LogTemp, Log, TEXT("[SignalJammer] Activated for %.1f seconds"), Duration);
}

void AEquippableUtility_SignalJammer::StopJammer()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsActive = false;

	// Collision 비활성화
	if (EffectSphere)
	{
		EffectSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 타이머 정리
	GetWorldTimerManager().ClearTimer(JammerTimerHandle);
	GetWorldTimerManager().ClearTimer(TickTimerHandle);

	// 이펙트 중지
	StopJammerEffects();

	UE_LOG(LogTemp, Log, TEXT("[SignalJammer] Deactivated"));

	// 1초 후 파괴
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(
		DestroyTimer,
		[this]() { Destroy(); },
		1.0f,
		false
	);
}

//==========================================
// EFFECTS
//==========================================

void AEquippableUtility_SignalJammer::ApplyJammerEffect()
{
	if (!bIsActive || !EffectSphere)
	{
		return;
	}

	// Sphere 내부의 모든 액터 가져오기
	TArray<AActor*> OverlappingActors;
	EffectSphere->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		// AI 캐릭터인지 확인 (AI Controller를 가진 캐릭터)
		ACharacter* Character = Cast<ACharacter>(Actor);
		if (Character && Character->GetController() && Character->GetController()->IsA(AAIController::StaticClass()))
		{
			// TODO: AI에게 디버프 적용
			// 예: 입력 반전, 이동 속도 감소 등
			// Character->ApplyInputReverseDebuff();

			UE_LOG(LogTemp, Log, TEXT("[SignalJammer] Jamming AI: %s"), *Character->GetName());
		}
	}
}

void AEquippableUtility_SignalJammer::StartJammerEffects()
{
	// 파티클 이펙트
	if (JammerParticle && !ActiveJammerParticle)
	{
		ActiveJammerParticle = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			JammerParticle,
			GetActorLocation(),
			FRotator::ZeroRotator,
			FVector(1.0f),
			true
		);
	}
}

void AEquippableUtility_SignalJammer::StopJammerEffects()
{
	if (ActiveJammerParticle)
	{
		ActiveJammerParticle->DeactivateSystem();
		ActiveJammerParticle = nullptr;
	}
}

//==========================================
// COLLISION CALLBACKS
//==========================================

void AEquippableUtility_SignalJammer::OnEffectSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bIsActive)
	{
		return;
	}

	// AI가 범위 안에 들어옴
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (Character && Character->GetController() && Character->GetController()->IsA(AAIController::StaticClass()))
	{
		UE_LOG(LogTemp, Log, TEXT("[SignalJammer] AI entered range: %s"), *Character->GetName());
	}
}

void AEquippableUtility_SignalJammer::OnEffectSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!bIsActive)
	{
		return;
	}

	// AI가 범위 밖으로 나감
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (Character && Character->GetController() && Character->GetController()->IsA(AAIController::StaticClass()))
	{
		UE_LOG(LogTemp, Log, TEXT("[SignalJammer] AI left range: %s"), *Character->GetName());
	}
}