
#include "TESTPlayerState.h"
#include "Net/UnrealNetwork.h"

void ATESTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATESTPlayerState, bisReady);
    DOREPLIFETIME(ATESTPlayerState, EOSPlayerName);
    DOREPLIFETIME(ATESTPlayerState, VoiceTeam);
}

void ATESTPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TryInitEOSPlayerName();
	}
}

void ATESTPlayerState::OnRep_VoiceTeam()
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceTeam] OnRep VoiceTeam=%d"), (int32)VoiceTeam);
}

void ATESTPlayerState::Server_SetVoiceTeam(EVoiceTeam NewTeam)
{
    if (!HasAuthority()) return;
    VoiceTeam = NewTeam;
    ForceNetUpdate();
}

void ATESTPlayerState::Server_SetEOSPlayerName(const FString& InEOSPlayerName)
{
	EOSPlayerName = InEOSPlayerName;
	ForceNetUpdate();
}

void ATESTPlayerState::OnRep_EOSPlayerName()
{
	UE_LOG(LogTemp, Log, TEXT("[TESTPlayerState] EOSPlayerName Replicated: %s"), *EOSPlayerName);
}





void ATESTPlayerState::TryInitEOSPlayerName()
{
    if (!EOSPlayerName.IsEmpty())
        return;

    if (!GetUniqueId().IsValid())
    {
        // 아직 UniqueId가 안 들어온 타이밍이면 잠깐 후 재시도
        GetWorldTimerManager().SetTimer(EOSNameInitTimer, this, &ATESTPlayerState::TryInitEOSPlayerName, 0.2f, false);
        return;
    }

    FString Raw = GetUniqueId()->ToString();  // 예: "EOS:xxxx|000209...."
    FString Left, Right;
    if (Raw.Split(TEXT("|"), &Left, &Right))
    {
        EOSPlayerName = Right; // ProductId만 저장
    }
    else
    {
        EOSPlayerName = Raw;
    }

    UE_LOG(LogTemp, Warning, TEXT("[TESTPlayerState] Set EOSPlayerName on SERVER: %s (Raw=%s)"),
        *EOSPlayerName, *Raw);

    ForceNetUpdate();
}