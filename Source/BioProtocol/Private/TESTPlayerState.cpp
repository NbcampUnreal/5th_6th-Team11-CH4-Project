
#include "TESTPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "MyTestGameInstance.h"

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
        // ✅ EOSPlayerName 초기화를 먼저 시작
        TryInitEOSPlayerName();
    }
}

void ATESTPlayerState::RestoreTeamFromGameInstance()
{
    // ✅ EOSPlayerName이 아직 없으면 재시도
    if (EOSPlayerName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] EOSPlayerName empty, retrying restore..."));
        FTimerHandle RetryTimer;
        GetWorldTimerManager().SetTimer(RetryTimer, this,
            &ATESTPlayerState::RestoreTeamFromGameInstance, 0.2f, false);
        return;
    }

    UMyTestGameInstance* GI = Cast<UMyTestGameInstance>(GetWorld()->GetGameInstance());
    if (!GI)
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerState] GameInstance is NULL"));
        return;
    }

    bool bFound = false;
    EVoiceTeam SavedTeam = GI->GetPlayerTeam(EOSPlayerName, bFound);

    if (bFound)
    {
        VoiceTeam = SavedTeam;
        ForceNetUpdate();

        UE_LOG(LogTemp, Warning, TEXT("[PlayerState] ✅ Restored team for %s: %s"),
            *EOSPlayerName,
            VoiceTeam == EVoiceTeam::Mafia ? TEXT("MAFIA") : TEXT("CITIZEN"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[PlayerState] No saved team for %s, using default: CITIZEN"),
            *EOSPlayerName);
    }
}

void ATESTPlayerState::OnRep_VoiceTeam()
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceTeam] OnRep VoiceTeam=%d"), (int32)VoiceTeam);
}

// ✅ 블루프린트에서 체크박스 클릭 시 호출
void ATESTPlayerState::Server_RequestTeamChange_Implementation(EVoiceTeam NewTeam)
{
    if (!HasAuthority())
        return;

    VoiceTeam = NewTeam;
    ForceNetUpdate();

    // ✅ GameInstance에 저장
    if (!EOSPlayerName.IsEmpty())
    {
        UMyTestGameInstance* GI = Cast<UMyTestGameInstance>(GetWorld()->GetGameInstance());
        if (GI)
        {
            GI->SavePlayerTeam(EOSPlayerName, NewTeam);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[TeamSelection] Player %s changed team to: %s"),
        *EOSPlayerName,
        NewTeam == EVoiceTeam::Mafia ? TEXT("MAFIA") : TEXT("CITIZEN"));
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
    {
        // ✅ EOSPlayerName이 이미 있으면 바로 팀 복원 시도
        RestoreTeamFromGameInstance();
        return;
    }

    if (!GetUniqueId().IsValid())
    {
        GetWorldTimerManager().SetTimer(EOSNameInitTimer, this,
            &ATESTPlayerState::TryInitEOSPlayerName, 0.2f, false);
        return;
    }

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

    UE_LOG(LogTemp, Warning, TEXT("[TESTPlayerState] Set EOSPlayerName on SERVER: %s (Raw=%s)"),
        *EOSPlayerName, *Raw);

    ForceNetUpdate();

    // ✅ EOSPlayerName 설정 직후 팀 복원
    RestoreTeamFromGameInstance();
}