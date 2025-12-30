#include "Daeho/TaskObject.h"
#include "Net/UnrealNetwork.h"
#include "Daeho/MyTestGameMode.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include <Game/BioGameState.h>
#include <Character/StaffCharacter.h>

ATaskObject::ATaskObject()
{
	bReplicates = true; // ?쒕쾭-?대씪 ?숆린???꾩닔
	bIsCompleted = false;

	// 湲곕낯媛? ?꾨Т ?꾧뎄??媛??(留⑥넀 誘몄뀡)
	RequiredTool = EToolType::None;

	// 1. 異⑸룎 諛뺤뒪 ?앹꽦 (猷⑦듃 而댄룷?뚰듃濡??ㅼ젙)
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	RootComponent = CollisionBox;

	// 諛뺤뒪 ?ш린 ?ㅼ젙 (?곷떦???ㅼ?)
	CollisionBox->SetBoxExtent(FVector(50.f, 50.f, 50.f));

	// 異⑸룎 ?ㅼ젙: Visibility 梨꾨꼸??留됱븘??LineTrace??嫄몃┝
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 2. ?섏씠?꾧???而댄룷?뚰듃 ?앹꽦
	NiagaraEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraEffect"));
	NiagaraEffect->SetupAttachment(RootComponent);

	// 湲곕낯?곸쑝濡??쒖꽦??
	NiagaraEffect->SetAutoActivate(true);
}

void ATaskObject::BeginPlay()
{
	Super::BeginPlay();

	// ?쒕쾭??寃쎌슦 GameMode???먯떊???깅줉 (?꾩껜 ?꾨Т ??利앷?)
	if (HasAuthority())
	{
		if (ABioGameState* GM = Cast<ABioGameState>(GetWorld()->GetGameState()))
		{
			GM->RegisterTask();
		}
	}

	// 寃뚯엫 以묎컙???ㅼ뼱???뚮젅?댁뼱瑜??꾪빐 珥덇린 ?곹깭??留욎떠 ?쒓컖 ?④낵 ?숆린??
	OnRep_IsCompleted();
}

void ATaskObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// bIsCompleted 蹂??蹂듭젣 ?깅줉
	DOREPLIFETIME(ATaskObject, bIsCompleted);
}

void ATaskObject::Interact_Implementation(APawn* InstigatorPawn)
{
	// ?쒕쾭?먯꽌留?濡쒖쭅 泥섎━
	if (!HasAuthority()) return;

	// ?대? ?꾨즺??
	if (bIsCompleted) return;

	//AStaffCharacter* MyChar = Cast<AStaffCharacter>(InstigatorPawn);
	ATestCharacter* MyChar = Cast<ATestCharacter>(InstigatorPawn);
	if (!MyChar) return;

	EToolType CharTool = MyChar->GetCurrentActiveTool();

	bool bCanDoTask = false;

	if (RequiredTool == EToolType::None)
	{
		// 留⑥넀 誘몄뀡? ?뚯튂???⑹젒湲곕? ?ㅺ퀬 ?덉뼱???섑뻾 媛??
		bCanDoTask = true;
	}
	else
	{
		if (CharTool == RequiredTool)
		{
			bCanDoTask = true;
		}
	}

	if (bCanDoTask)
	{
		bIsCompleted = true;

		/*if (ABioGameState* GM = Cast<ABioGameState>(GetWorld()->GetGameState()))
		{
			GM->AddMissionProgress(1);
		}*/

		if (AMyTestGameMode* GM = Cast<AMyTestGameMode>(GetWorld()->GetGameState()))
		{
			GM->OnTaskCompleted();
		}

		OnRep_IsCompleted();

		if (RequiredTool != EToolType::None)
		{
			MyChar->ConsumeToolDurability(1);
		}

		UE_LOG(LogTemp, Log, TEXT("Task Done!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Task Failed!. Required Tool: %d, Has: %d"),
			(int32)RequiredTool, (int32)MyChar->GetCurrentActiveTool());
	}
}

void ATaskObject::OnRep_IsCompleted()
{
	// ?꾨Т ?꾨즺 ???됱긽??珥덈줉?됱쑝濡?諛붽씀嫄곕굹 硫붿떆吏瑜??꾩슦?????쒓컖??泥섎━
	if (bIsCompleted)
	{
		if (NiagaraEffect)
		{
			// ?댄럺??鍮꾪솢?깊솕 (?щ씪吏?
			NiagaraEffect->Deactivate();

			// ?뱀? ?④? 泥섎━
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


