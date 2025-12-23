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
    SessionSettings.bUseLobbiesIfAvailable = true; // EOS/Steam 등에서 Lobby 사용
    SessionSettings.bUseLobbiesVoiceChatIfAvailable = true;


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

    UE_LOG(LogTemp, Warning, TEXT("[Lobby][Create][Pre] LoginStatus=%d UserIdValid=%d UserId=%s"), (int32)Status, UserId.IsValid() ? 1 : 0, UserId.IsValid() ? *UserId->ToString() : TEXT("NULL"));

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
    {
        return;
    }


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
    Settings.bIsLANMatch = false;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;

    // EOS 로비, 보이스 자동 조인
    Settings.bUseLobbiesIfAvailable = true;
    Settings.bUseLobbiesVoiceChatIfAvailable = true;

    // presence
    Settings.bUsesPresence = true;
    Settings.bAllowJoinViaPresence = true;

    Settings.NumPublicConnections = PublicConnections;

    // 로비에 DS 주소 입력
    Settings.Set(SEARCH_LOBBIES, true, EOnlineDataAdvertisementType::ViaOnlineService);

    const FString ServerAddr = FString::Printf(TEXT("%s:%d"), *ServerIp, ServerPort);
    Settings.Set(FName("SERVER_ADDR"), ServerAddr, EOnlineDataAdvertisementType::ViaOnlineService);

    Settings.Set(FName("SERVER_IP"), ServerIp, EOnlineDataAdvertisementType::ViaOnlineService);
    Settings.Set(FName("SERVER_PORT"), FString::FromInt(ServerPort), EOnlineDataAdvertisementType::ViaOnlineService);

    // 검색 필터용 태그
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

    if (SessionInterface.IsValid())
    {
        SessionInterface->StartSession(SessionName);
    }

    TravelToDedicatedFromLobby(SessionName);
}

// 세션 검색
void USessionSubsystem::FindGameSessions(int32 MaxResults, bool bIsLAN)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (IsRunningDedicatedServer())
    {
        return;
    }

    // 1) 로그인 준비 체크 (CreateLobbyForDedicated랑 동일 패턴)
    IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);
    if (!Identity.IsValid())
    {
        return;
    }

    const int32 LocalUserNum = 0;
    const ELoginStatus::Type Status = Identity->GetLoginStatus(LocalUserNum);
    TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalUserNum);

    if (Status != ELoginStatus::LoggedIn || !UserId.IsValid())
    {
        FTimerHandle Tmp;
        World->GetTimerManager().SetTimer(Tmp, [this, MaxResults, bIsLAN]()
            {
                this->FindGameSessions(MaxResults, bIsLAN);
            }, 0.1f, false);
        return;
    }

    SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid()) return;

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->MaxSearchResults = MaxResults;
    SessionSearch->bIsLanQuery = bIsLAN; 

    SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
    SessionSearch->QuerySettings.Set(FName("LOBBY_TAG"), FString("BIO"), EOnlineComparisonOp::Equals);
    // SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

    OnFindSessionsCompleteHandle =
        SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
            FOnFindSessionsCompleteDelegate::CreateUObject(
                this, &USessionSubsystem::HandleFindSessionsComplete));

    UE_LOG(LogTemp, Warning, TEXT("[Find] bIsLAN=%d UserIdValid=%d"), bIsLAN, UserId.IsValid() ? 1 : 0);

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

            FString Addr;
            Result.Session.SessionSettings.Get(FName("SERVER_ADDR"), Addr);

            FString Ip, PortPart;
            int32 Port = 0;
            if (Addr.Split(TEXT(":"), &Ip, &PortPart))
            {
                Port = FCString::Atoi(*PortPart);
            }

            Info.ServerIp = Ip;
            Info.ServerPort = Port;

            LastSessionInfos.Add(Info);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Lobby][FindComplete] ok=%d results=%d"),
        bWasSuccessful, SessionSearch->SearchResults.Num());

    for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
    {
        const FOnlineSessionSearchResult& R = SessionSearch->SearchResults[i];

        FString Ip;
        int32 PortInt = -1;
        FString PortStr;
        FString Addr;

        const bool bHasIp = R.Session.SessionSettings.Get(FName("SERVER_IP"), Ip);
        const bool bHasPortInt = R.Session.SessionSettings.Get(FName("SERVER_PORT"), PortInt);
        const bool bHasPortStr = R.Session.SessionSettings.Get(FName("SERVER_PORT"), PortStr);
        const bool bHasAddr = R.Session.SessionSettings.Get(FName("SERVER_ADDR"), Addr);

        UE_LOG(LogTemp, Warning,
            TEXT("[Lobby][Result %d] Ping=%d Open=%d/%d HasIp=%d Ip='%s' HasPortInt=%d PortInt=%d HasPortStr=%d PortStr='%s' HasAddr=%d Addr='%s'"),
            i,
            R.PingInMs,
            R.Session.NumOpenPublicConnections,
            R.Session.SessionSettings.NumPublicConnections,
            bHasIp ? 1 : 0, *Ip,
            bHasPortInt ? 1 : 0, PortInt,
            bHasPortStr ? 1 : 0, *PortStr,
            bHasAddr ? 1 : 0, *Addr
        );

        // SessionSettings 전체 덤프 (여기서 SERVER_*가 실제로 존재하는지 바로 보임)
        UE_LOG(LogTemp, Warning, TEXT("[Lobby][Result %d] --- Settings Dump ---"), i);
        for (const auto& Pair : R.Session.SessionSettings.Settings)
        {
            const FString Val = Pair.Value.Data.ToString(); // 반환값을 받아야 함
            UE_LOG(LogTemp, Warning, TEXT(" Key='%s' Val='%s'"), *Pair.Key.ToString(), *Val);
        }
    }

    OnSessionSearchUpdated.Broadcast(LastSessionInfos);
}

// 데디케이트 서버 게임 시작
void USessionSubsystem::StartGameSession()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[StartGameSession] No World"));
        return;
    }

    // ✅ 서버에서만 실행
    if (!World->GetAuthGameMode())
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartGameSession] Not on server, ignored"));
        return;
    }

    // ✅ 현재 맵 확인
    FString CurrentMap = World->GetMapName();
    UE_LOG(LogTemp, Warning, TEXT("[StartGameSession] Current Map: %s"), *CurrentMap);

    // ✅ 이미 게임맵에 있으면 중복 Travel 방지
    if (CurrentMap.Contains(TEXT("TestSession")))
    {
        UE_LOG(LogTemp, Warning, TEXT("[StartGameSession] Already in TestSession, skipping travel"));
        return;
    }

    FString TravelURL = TEXT("/Game/Level/TestSession");
    UE_LOG(LogTemp, Warning, TEXT("[StartGameSession] ServerTravel to: %s"), *TravelURL);

    World->ServerTravel(TravelURL, false); // ✅ Seamless=false (절대 경로)
}

// 로비 생성 완료시 그 로비로 이동
void USessionSubsystem::TravelToDedicatedFromLobby(const FName& LobbySessionName)
{
    if (!SessionInterface.IsValid() || !GetWorld())
    {
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    FNamedOnlineSession* Named = SessionInterface->GetNamedSession(LobbySessionName);
    if (!Named)
    {
        UE_LOG(LogTemp, Error, TEXT("[TravelToDedicatedFromLobby] No named session"));
        return;
    }

    FString Addr;

    if (!Named->SessionSettings.Get(FName("SERVER_ADDR"), Addr) || Addr.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[TravelToDedicatedFromLobby] SERVER_ADDR missing. Can't travel."));
        return;
    }

    if (!Addr.Contains(TEXT(":")))
    {
        Addr = FString::Printf(TEXT("%s:%d"), *Addr, 7777);
    }

    UE_LOG(LogTemp, Warning, TEXT("[TravelToDedicatedFromLobby] Travel to '%s'"), *Addr);

    if (PC)
    {
        PC->ClientTravel(Addr, ETravelType::TRAVEL_Absolute);
    }
}

// 발견한 로비로 Join
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

    // index 로비로
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

    UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] HandleJoinSessionComplete: %s, Result: %d"), *SessionName.ToString(), (int32)Result);

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
    const FName LobbySessionName = TEXT("LobbySession");
    TravelToDedicatedFromLobby(LobbySessionName);
}