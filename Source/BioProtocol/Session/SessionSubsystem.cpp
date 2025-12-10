// Fill out your copyright notice in the Description page of Project Settings.


#include "SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"


void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // OnlineSubsystem 가져오기 (Null, Steam, EOS 등)
    if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
    {
        SessionInterface = Subsystem->GetSessionInterface();

        if (SessionInterface.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("[MySessionSubsystem] OnlineSubsystem: %s"),
                *Subsystem->GetSubsystemName().ToString());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[MySessionSubsystem] SessionInterface is invalid"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[MySessionSubsystem] No OnlineSubsystem found"));
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
    SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] SessionInterface invalid in CreateGameSession"));
        return;
    }

    // 기존 세션이 있으면 삭제
    FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("[MySessionSubsystem] Destroy existing session before creating new one"));
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
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] No PlayerController in CreateGameSession"));
        return;
    }

    const int32 LocalUserNum = 0; // 일단 0번으로

    bool bCreateResult = SessionInterface->CreateSession(LocalUserNum, NAME_GameSession, SessionSettings);

    if (!bCreateResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[MySessionSubsystem] CreateSession failed to start"));
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

    UE_LOG(LogTemp, Log, TEXT("[MySessionSubsystem] HandleCreateSessionComplete: %s, Success: %d"),
        *SessionName.ToString(), bWasSuccessful);

    if (!bWasSuccessful)
    {
        return;
    }

    // 세션 생성에 성공했다면, 호스트가 맵을 listen 서버로 열기
    //UWorld* World = GetWorld();
    //if (!World) return;

    // 예시: "GameMap?listen"
    //FString TravelURL = TEXT("/Game/Maps/GameMap?listen");
    //World->ServerTravel(TravelURL);
}

// 세션 검색
void USessionSubsystem::FindGameSessions(int32 MaxResults, bool bIsLAN)
{
    SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] SessionInterface invalid in FindGameSessions"));
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
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] No PlayerController in FindGameSessions"));
        return;
    }

    const int32 LocalUserNum = 0;

    bool bFindResult = SessionInterface->FindSessions(LocalUserNum, SessionSearch.ToSharedRef());

    if (!bFindResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[MySessionSubsystem] FindSessions failed to start"));
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

    UE_LOG(LogTemp, Log, TEXT("[MySessionSubsystem] HandleFindSessionsComplete: Success: %d, NumResults: %d"),
        bWasSuccessful,
        SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);

    if (!bWasSuccessful || !SessionSearch.IsValid())
    {
        return;
    }

    // 여기서 SessionSearch->SearchResults 배열을 사용해서
    // UI에 리스트 뜨게 하거나, 이번 예제처럼 첫 번째 세션에 바로 조인할 수 있음.
}


void USessionSubsystem::JoinFoundSession()
{
    SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] JoinFirstFoundSession: invalid state"));
        return;
    }

    if (SessionSearch->SearchResults.Num() <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] No sessions found to join"));
        return;
    }

    // 일단 첫 번째 세션으로
    const FOnlineSessionSearchResult& FirstResult = SessionSearch->SearchResults[0];

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
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] No PlayerController in JoinFirstFoundSession"));
        return;
    }

    const int32 LocalUserNum = 0;

    bool bJoinResult = SessionInterface->JoinSession(LocalUserNum, NAME_GameSession, FirstResult);

    if (!bJoinResult)
    {
        UE_LOG(LogTemp, Error, TEXT("[MySessionSubsystem] JoinSession failed to start"));
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }
}

void USessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }

    UE_LOG(LogTemp, Log, TEXT("[MySessionSubsystem] HandleJoinSessionComplete: %s, Result: %d"),
        *SessionName.ToString(), (int32)Result);

    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] JoinSession not successful"));
        return;
    }

    // 접속할 URL 얻기
    FString ConnectString;
    if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
    {
        UE_LOG(LogTemp, Error, TEXT("[MySessionSubsystem] Could not get connect string"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[MySessionSubsystem] ConnectString: %s"), *ConnectString);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MySessionSubsystem] No PlayerController in HandleJoinSessionComplete"));
        return;
    }

    // 클라에서 서버로 여행
    PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
}