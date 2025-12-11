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
    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        SessionInterface = Subsystem->GetSessionInterface();

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
    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] Destroy existing session before creating new one"));
        SessionInterface->DestroySession(NAME_GameSession);
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

    SessionSettings.bAllowJoinViaPresence = false;
    SessionSettings.bUsesPresence = false;
    SessionSettings.bUseLobbiesIfAvailable = false; // EOS/Steam 등에서 Lobby 사용

    // 로컬 플레이어(0번)로 세션 생성
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] No PlayerController in CreateGameSession"));
        return;
    }

    const int32 LocalUserNum = 0; // 일단 0번으로

    bool bCreateResult = SessionInterface->CreateSession(LocalUserNum, NAME_GameSession, SessionSettings);

    if (!bCreateResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] CreateSession failed to start"));
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
    }
}

// 세선 생성 완료 시
void USessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    // 델리게이트 해제
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] HandleCreateSessionComplete: %s, Success: %d"),
        *SessionName.ToString(), bWasSuccessful);

    if (!bWasSuccessful)
    {
        return;
    }

    //UWorld* World = GetWorld();
    //if (!World) return;

    //FString TravelURL = TEXT("/Game/Level/TestSession?listen");
    //World->ServerTravel(TravelURL);
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
    SessionSearch->bIsLanQuery = bIsLAN;
    // Presence 사용 세션만 찾기
    //SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

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

            LastSessionInfos.Add(Info);
        }
    }

    OnSessionSearchUpdated.Broadcast(LastSessionInfos);
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

    bool bJoinResult = SessionInterface->JoinSession(LocalUserNum, NAME_GameSession, FirstResult);

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
    FString ConnectString;
    if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
    {
        UE_LOG(LogTemp, Error, TEXT("[SessionSubsystem] Could not get connect string"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SessionSubsystem] ConnectString: %s"), *ConnectString);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionSubsystem] No PlayerController in HandleJoinSessionComplete"));
        return;
    }

    // 클라에서 서버로 여행
    PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}