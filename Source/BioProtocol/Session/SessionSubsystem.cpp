// Fill out your copyright notice in the Description page of Project Settings.


#include "SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"


void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // OnlineSubsystem 가져오기 (Null, Steam, EOS 등)
    if (UWorld* World = GetWorld())
    {
        if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
        {
            SessionInterface = GetSessionInterface();

            if (SessionInterface.IsValid())
            {
                UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] OnlineSubsystem: %s"),
                    *Subsystem->GetSubsystemName().ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] SessionInterface is invalid"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] No OnlineSubsystem found"));
        }
    }

}

void USessionSubsystem::Deinitialize()
{
    // 필요 시 델리게이트 언바인드 등 정리
    SessionInterface.Reset();

    Super::Deinitialize();
}

IOnlineSessionPtr USessionSubsystem::GetSessionInterface() const
{
    if (UWorld* World = GetWorld())
    {
        return Online::GetSessionInterface(World);
    }
    return nullptr;
}



// 세선 생성
void USessionSubsystem::CreateGameSession(int32 PublicConnections, bool bIsLAN)
{
    if (!GetWorld() || !GetWorld()->GetAuthGameMode())
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateGameSession called but not on server"));
        return;
    }



    SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] SessionInterface invalid in CreateGameSession"));
        return;
    }

    // 기존 세션이 있으면 삭제
    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(TEXT("LobbySession"));
    if (ExistingSession != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] Destroy existing session before creating new one"));
        SessionInterface->DestroySession(TEXT("LobbySession"));
    }

    // 델리게이트 바인딩
    OnCreateSessionCompleteHandle =
        SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
            FOnCreateSessionCompleteDelegate::CreateUObject(
                this, &USessionSubsystem::HandleCreateSessionComplete));

    // 세션 설정
    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsLANMatch = bIsLAN;
    SessionSettings.NumPublicConnections = PublicConnections;
    SessionSettings.bShouldAdvertise = true; // FindSessions로 찾을 수 있도록 광고
    SessionSettings.bAllowJoinInProgress = true;

    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bUseLobbiesIfAvailable = false; // EOS/Steam 등에서 Lobby 사용


    const int32 LocalUserNum = 0;

    FName LobbySessionName = TEXT("LobbySession");

    bool bCreateResult = SessionInterface->CreateSession(LocalUserNum, LobbySessionName, SessionSettings);

    if (!bCreateResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] CreateSession failed to start"));
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
    }
}

void USessionSubsystem::CreateLobbyForDedicated(const FString& ServerIp, int32 ServerPort, int32 PublicConnections)
{
    UWorld* World = GetWorld();
    UE_LOG(LogTemp, Warning, TEXT("[Lobby][Create][Pre] World=%s NetMode=%d IsDS=%d HasAuthGameMode=%d"),
        World ? *World->GetName() : TEXT("NULL"),
        World ? (int32)World->GetNetMode() : -1,
        (int32)IsRunningDedicatedServer(),
        World && World->GetAuthGameMode() ? 1 : 0);

    // 로컬 플레이어가 없으면(=서버 컨텍스트) 로비 만들면 안 됨
    if (!World || !World->GetFirstPlayerController() || !World->GetFirstPlayerController()->IsLocalController())
    {
        UE_LOG(LogTemp, Error, TEXT("[Lobby][Create][Pre] No LocalController -> call this on HOST LOCAL client only"));
        return;
    }

    IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[Lobby][Create][Pre] Identity invalid"));
        return;
    }

    const int32 LocalUserNum = 0;
    const ELoginStatus::Type Status = Identity->GetLoginStatus(LocalUserNum);
    TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalUserNum);

    UE_LOG(LogTemp, Warning, TEXT("[Lobby][Create][Pre] LoginStatus=%d UserIdValid=%d UserId=%s"),
        (int32)Status, UserId.IsValid() ? 1 : 0, UserId.IsValid() ? *UserId->ToString() : TEXT("NULL"));

    if (Status != ELoginStatus::LoggedIn || !UserId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[Lobby][Create][Pre] Not logged in for sessions yet -> retry next tick"));
        // 타이밍 문제면 다음 틱에 다시 시도
        FTimerHandle Tmp;
        World->GetTimerManager().SetTimer(Tmp, [this, ServerIp, ServerPort, PublicConnections]()
            {
                this->CreateLobbyForDedicated(ServerIp, ServerPort, PublicConnections);
            }, 0.1f, false);
        return;
    }
    if (IsRunningDedicatedServer())
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateLobbyForDedicated called on dedicated server – ignored"));
        return;
    }

    SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid())
        return;



    const FName LobbySessionName(TEXT("LobbySession"));

    // 기존 로비 세션이 있으면 삭제
    if (SessionInterface->GetNamedSession(LobbySessionName))
    {
        SessionInterface->DestroySession(LobbySessionName);
    }

    OnCreateSessionCompleteHandle =
        SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
            FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::HandleCreateSessionComplete));

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = false;                 // EOS에 광고할 거면 false
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;

    // 여기부터가 “EOS 로비 + 보이스 자동 조인”용 핵심 스위치
    Settings.bUseLobbiesIfAvailable = true;
    Settings.bUseLobbiesVoiceChatIfAvailable = true;

    // 보통 EOS 검색 안정성 때문에 presence도 같이 켜는 편이 좋음
    Settings.bUsesPresence = true;
    Settings.bAllowJoinViaPresence = true;

    Settings.NumPublicConnections = PublicConnections;

    // 로비에 DS 주소 박기 (핵심)
    Settings.Set(FName("SERVER_IP"), ServerIp, EOnlineDataAdvertisementType::ViaOnlineService);
    Settings.Set(FName("SERVER_PORT"), ServerPort, EOnlineDataAdvertisementType::ViaOnlineService);

    // 검색 필터용 태그(선택이지만 강추: 다른 게임/테스트 로비랑 섞이는 걸 방지)
    Settings.Set(FName("LOBBY_TAG"), FString("BIO"), EOnlineDataAdvertisementType::ViaOnlineService);


    UE_LOG(LogTemp, Log, TEXT("[Lobby][Create] ip=%s port=%d pub=%d (UseLobbies=%d Voice=%d Presence=%d Tag=BIO)"),
        *ServerIp, ServerPort, PublicConnections,
        Settings.bUseLobbiesIfAvailable,
        Settings.bUseLobbiesVoiceChatIfAvailable,
        Settings.bUsesPresence);

    const bool bStarted = SessionInterface->CreateSession(LocalUserNum, LobbySessionName, Settings);
    UE_LOG(LogTemp, Log, TEXT("[Lobby][Create] CreateSession started=%d"), bStarted);
}


// 세선 생성 완료 시
void USessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    // 델리게이트 해제
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
    }

    UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] HandleCreateSessionComplete: %s, Success: %d"),
        *SessionName.ToString(), bWasSuccessful);

    if (!bWasSuccessful)
    {
        return;
    }
}

// 세션 검색
void USessionSubsystem::FindGameSessions(int32 MaxResults, bool bIsLAN)
{
    if (IsRunningDedicatedServer())
    {
        UE_LOG(LogTemp, Warning, TEXT("FindGameSessions called on dedicated server – ignored"));
        return;
    }

    SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] SessionInterface invalid in FindGameSessions"));
        return;
    }

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->MaxSearchResults = MaxResults;
    SessionSearch->bIsLanQuery = false;

    // Presence 사용 세션만 찾기
    SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
    SessionSearch->QuerySettings.Set(FName("LOBBY_TAG"), FString("BIO"), EOnlineComparisonOp::Equals);

    OnFindSessionsCompleteHandle =
        SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
            FOnFindSessionsCompleteDelegate::CreateUObject(
                this, &USessionSubsystem::HandleFindSessionsComplete));

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] No PlayerController in FindGameSessions"));
        return;
    }

    const int32 LocalUserNum = 0;

    bool bFindResult = SessionInterface->FindSessions(LocalUserNum, SessionSearch.ToSharedRef());

    if (!bFindResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] FindSessions failed to start"));
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
    }
}

// 세션 검색 완료 시
void USessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] HandleFindSessionsComplete: Success: %d, NumResults: %d"),
        bWasSuccessful,
        SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);

    LastSessionInfos.Empty();

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
        {
            const auto& Result = SessionSearch->SearchResults[i];

            FSessionInfo Info;
            Info.SearchResultIndex = i;
            Info.PingInMs = Result.PingInMs;

            // 세션 이름
            FString Name;
            if (!Result.Session.SessionSettings.Get(FName(TEXT("SESSION_NAME")), Name))
            {
                Name = FString::Printf(TEXT("Session_%d"), i);
            }
            Info.SessionName = Name;

            // 인원
            const int32 OpenSlots = Result.Session.NumOpenPublicConnections;
            const int32 MaxSlots = Result.Session.SessionSettings.NumPublicConnections;
            Info.MaxPlayers = MaxSlots;
            Info.CurrentPlayers = MaxSlots - OpenSlots;

            FString Ip;
            int32 Port = 0;
            Result.Session.SessionSettings.Get(FName("SERVER_IP"), Ip);
            Result.Session.SessionSettings.Get(FName("SERVER_PORT"), Port);

            Info.ServerIp = Ip;
            Info.ServerPort = Port;

            LastSessionInfos.Add(Info);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Lobby][FindComplete] ok=%d results=%d"),
    bWasSuccessful, SessionSearch->SearchResults.Num());

    for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
    {
        const auto& R = SessionSearch->SearchResults[i];

        FString Ip;
        int32 Port = 0;
        R.Session.SessionSettings.Get(FName("SERVER_IP"), Ip);
        R.Session.SessionSettings.Get(FName("SERVER_PORT"), Port);

        UE_LOG(LogTemp, Log, TEXT("[Lobby][Result %d] Ping=%d Open=%d/%d ip=%s port=%d"),
            i,
            R.PingInMs,
            R.Session.NumOpenPublicConnections,
            R.Session.SessionSettings.NumPublicConnections,
            *Ip, Port);
    }
    OnSessionSearchUpdated.Broadcast(LastSessionInfos);
}

void USessionSubsystem::StartGameSession()
{

    if (!GetWorld() || !GetWorld()->GetAuthGameMode())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] StartGameSession called on client"));
        return;
    }

    SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] SessionInterface invalid in FindGameSessions"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    FString TravelURL = TEXT("/Game/Level/TestSession");
    World->ServerTravel(TravelURL);
}

void USessionSubsystem::JoinFoundSession(int32 index)
{
    if (IsRunningDedicatedServer())
    {
        UE_LOG(LogTemp, Warning, TEXT("FindGameSessions called on dedicated server – ignored"));
        return;
    }

    SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] JoinFirstFoundSession: invalid state"));
        return;
    }

    if (SessionSearch->SearchResults.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] No sessions found to join"));
        return;
    }

    // 일단 첫 번째 세션으로
    const FOnlineSessionSearchResult& FirstResult = SessionSearch->SearchResults[index];

    OnJoinSessionCompleteHandle =
        SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
            FOnJoinSessionCompleteDelegate::CreateUObject(
                this, &USessionSubsystem::HandleJoinSessionComplete));

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] No PlayerController in JoinFirstFoundSession"));
        return;
    }

    const int32 LocalUserNum = 0;

    const FName LobbySessionName = TEXT("LobbySession");

    bool bJoinResult = SessionInterface->JoinSession(LocalUserNum, LobbySessionName, FirstResult);

    if (!bJoinResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] JoinSession failed to start"));
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }
}

void USessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] HandleJoinSessionComplete: %s, Result: %d"),
        *SessionName.ToString(), (int32)Result);

    EJoinResultBP BPResult = EJoinResultBP::UnknownError;

    switch (Result)
    {
    case EOnJoinSessionCompleteResult::Success:
        BPResult = EJoinResultBP::Success;
        break;
    case EOnJoinSessionCompleteResult::SessionIsFull:
        BPResult = EJoinResultBP::SessionIsFull;
        break;
    case EOnJoinSessionCompleteResult::SessionDoesNotExist:
        BPResult = EJoinResultBP::SessionDoesNotExist;
        break;
    case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
        BPResult = EJoinResultBP::CouldNotRetrieveAddress;
        break;
    case EOnJoinSessionCompleteResult::AlreadyInSession:
        BPResult = EJoinResultBP::AlreadyInSession;
        break;
    default:
        BPResult = EJoinResultBP::UnknownError;
        break;
    }

    // 여기서 BP에게 결과 알림
    OnJoinSessionFinished.Broadcast(BPResult);

    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] JoinSession not successful"));
        return;
    }

    // 접속할 URL 얻기
    const FName LobbySessionName(TEXT("LobbySession"));
    FNamedOnlineSession* Named = SessionInterface->GetNamedSession(LobbySessionName);
    if (!Named)
    {
        UE_LOG(LogTemp, Error, TEXT("No named lobby session"));
        return;
    }

    FString Ip;
    int32 Port = 0;
    if (!Named->SessionSettings.Get(FName("SERVER_IP"), Ip) ||
        !Named->SessionSettings.Get(FName("SERVER_PORT"), Port))
    {
        UE_LOG(LogTemp, Error, TEXT("No SERVER_IP / SERVER_PORT in lobby settings"));
        return;
    }

    const FString Url = FString::Printf(TEXT("%s:%d"), *Ip, Port);

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        PC->ClientTravel(Url, ETravelType::TRAVEL_Absolute);
    }
}