#include "TESTPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "MyTestGameInstance.h"

//void ATESTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
//{
//    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//    DOREPLIFETIME(ATESTPlayerState, bisReady);
//    DOREPLIFETIME(ATESTPlayerState, EOSPlayerName);
//    DOREPLIFETIME(ATESTPlayerState, VoiceTeam);
//}
//
//void ATESTPlayerState::BeginPlay()
//{
//    Super::BeginPlay();
//
//    if (HasAuthority())
//    {
//        TryInitEOSPlayerName();
//    }
//}
//
//void ATESTPlayerState::RestoreTeamFromGameInstance()
//{
//    if (EOSPlayerName.IsEmpty())
//    {
//        FTimerHandle RetryTimer;
//        GetWorldTimerManager().SetTimer(RetryTimer, this,
//            &ATESTPlayerState::RestoreTeamFromGameInstance, 0.2f, false);
//        return;
//    }
//
//    UMyTestGameInstance* GI = Cast<UMyTestGameInstance>(GetWorld()->GetGameInstance());
//    if (!GI)
//    {
//        return;
//    }
//
//    bool bFound = false;
//    EVoiceTeam SavedTeam = GI->GetPlayerTeam(EOSPlayerName, bFound);
//
//    if (bFound)
//    {
//        VoiceTeam = SavedTeam;
//        ForceNetUpdate();
//    }
//}
//
//void ATESTPlayerState::OnRep_VoiceTeam()
//{
//}
//
//void ATESTPlayerState::Server_RequestTeamChange_Implementation(EVoiceTeam NewTeam)
//{
//    if (!HasAuthority())
//        return;
//
//    VoiceTeam = NewTeam;
//    ForceNetUpdate();
//
//    if (!EOSPlayerName.IsEmpty())
//    {
//        UMyTestGameInstance* GI = Cast<UMyTestGameInstance>(GetWorld()->GetGameInstance());
//        if (GI)
//        {
//            GI->SavePlayerTeam(EOSPlayerName, NewTeam);
//        }
//    }
//}
//
//void ATESTPlayerState::Server_SetVoiceTeam(EVoiceTeam NewTeam)
//{
//    if (!HasAuthority()) return;
//    VoiceTeam = NewTeam;
//    ForceNetUpdate();
//}
//
//void ATESTPlayerState::Server_SetEOSPlayerName(const FString& InEOSPlayerName)
//{
//    EOSPlayerName = InEOSPlayerName;
//    ForceNetUpdate();
//}
//
//void ATESTPlayerState::OnRep_EOSPlayerName()
//{
//}
//
//void ATESTPlayerState::TryInitEOSPlayerName()
//{
//    if (!EOSPlayerName.IsEmpty())
//    {
//        RestoreTeamFromGameInstance();
//        return;
//    }
//
//    if (!GetUniqueId().IsValid())
//    {
//        GetWorldTimerManager().SetTimer(EOSNameInitTimer, this,
//            &ATESTPlayerState::TryInitEOSPlayerName, 0.2f, false);
//        return;
//    }
//
//    FString Raw = GetUniqueId()->ToString();
//    FString Left, Right;
//    if (Raw.Split(TEXT("|"), &Left, &Right))
//    {
//        EOSPlayerName = Right;
//    }
//    else
//    {
//        EOSPlayerName = Raw;
//    }
//
//    ForceNetUpdate();
//    RestoreTeamFromGameInstance();
//}