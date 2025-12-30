#include "Daeho/TaskObject.h"
#include "Net/UnrealNetwork.h"
#include "Daeho/MyTestGameMode.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include <Game/BioGameState.h>
#include <Character/StaffCharacter.h>

ATaskObject::ATaskObject()
{
	bReplicates = true; // 서버-클라 동기화 필수
	bIsCompleted = false;

	// 기본값: 아무 도구나 가능 (맨손 미션)
	RequiredTool = EToolType::None;

	// 1. 충돌 박스 생성 (루트 컴포넌트로 설정)
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	// 박스 크기 설정 (적당히 키움)
	CollisionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

	// 충돌 설정: Visibility 채널을 막아야 LineTrace에 걸림
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 2. 나이아가라 컴포넌트 생성
	NiagaraEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraEffect"));
	NiagaraEffect->SetupAttachment(RootComponent);

	// 기본적으로 활성화
	NiagaraEffect->SetAutoActivate(true);
}

void ATaskObject::BeginPlay()
{
	Super::BeginPlay();

	// 서버인 경우 GameMode에 자신을 등록 (전체 임무 수 증가)
	if (HasAuthority())
	{
		if (ABioGameState* GM = Cast<ABioGameState>(GetWorld()->GetGameState()))
		{
			GM->RegisterTask();
		}
	}

	// 게임 중간에 들어온 플레이어를 위해 초기 상태에 맞춰 시각 효과 동기화
	OnRep_IsCompleted();
}

void ATaskObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// bIsCompleted 변수 복제 등록
	DOREPLIFETIME(ATaskObject, bIsCompleted);
}

void ATaskObject::Interact_Implementation(APawn* InstigatorPawn)
{
	// 서버에서만 로직 처리
	if (!HasAuthority()) return;

	// 이미 완료됨
	if (bIsCompleted) return;

	// 상호작용 시도한 캐릭터 가져오기
	AStaffCharacter* MyChar = Cast<AStaffCharacter>(InstigatorPawn);
	if (!MyChar) return;

	bool bCanDoTask = false;

	if (RequiredTool == EToolType::None)
	{
		// 맨손 미션은 렌치나 용접기를 들고 있어도 수행 가능
		bCanDoTask = true;
	}
	else
	{
		// 특정 도구 미션은 해당 도구를 들고 있어야 함
		if (MyChar->GetCurrentTool() == RequiredTool)
		{
			bCanDoTask = true;
		}
	}

	if (bCanDoTask)
	{
		// 임무 성공
		bIsCompleted = true;

		if (ABioGameState* GM = Cast<ABioGameState>(GetWorld()->GetGameState()))
		{
			GM->AddMissionProgress(1);
		}

		OnRep_IsCompleted();

		UE_LOG(LogTemp, Log, TEXT("도구가 일치합니다! 임무 완료!"));
	}
	else
	{
		// 도구 불일치 실패 로그
		UE_LOG(LogTemp, Warning, TEXT("임무 실패: 도구가 없습니다. Required: %d, Has: %d"),
			(int32)RequiredTool, (int32)MyChar->GetCurrentTool());
	}
}

void ATaskObject::OnRep_IsCompleted()
{
	// 임무 완료 시 색상을 초록색으로 바꾸거나 메시지를 띄우는 등 시각적 처리
	if (bIsCompleted)
	{
		if (NiagaraEffect)
		{
			// 이펙트 비활성화 (사라짐)
			NiagaraEffect->Deactivate();

			// 혹은 숨김 처리
			NiagaraEffect->SetVisibility(false);
		}

		UE_LOG(LogTemp, Log, TEXT("Task Completed Visual Update!"));
	}
	else
	{
		if (NiagaraEffect)
		{
			NiagaraEffect->Activate(true);
			NiagaraEffect->SetVisibility(true);
		}
	}
}


