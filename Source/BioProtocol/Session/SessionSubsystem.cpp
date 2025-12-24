#include "SessionSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(World))
        {
            SessionInterface = GetSessionInterface();
        }
    }
}

void USessionSubsystem::Deinitialize()
{
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

void USessionSubsystem::CreateLobbyForDedicated(const FString& ServerIp, int32 ServerPort, int32 PublicConnections)
{
    UWorld* World = GetWorld();
    if (!World || !World->GetFirstPlayerController() || !World->GetFirstPlayerController()->IsLocalController())
    {
        UE_LOG(LogTemp, Error, TEXT("[Session] CreateLobby requires local controller"));
        return;
    }

    IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);
    if (!Identity.IsValid())
    {
        return;
    }

    const int32 LocalUserNum = 0;
    if (Identity->GetLoginStatus(LocalUserNum) != ELoginStatus::LoggedIn || !Identity->GetUniquePlayerId(LocalUserNum).IsValid())
    {
        FTimerHandle Tmp;
        World->GetTimerManager().SetTimer(Tmp, [this, ServerIp, ServerPort, PublicConnections]()
            {
                this->CreateLobbyForDedicated(ServerIp, ServerPort, PublicConnections);
            }, 0.1f, false);
        return;
    }

    if (IsRunningDedicatedServer())
    {
        return;
    }

    SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        return;
    }

    const FName LobbySessionName(TEXT("LobbySession"));

    if (SessionInterface->GetNamedSession(LobbySessionName))
    {
        SessionInterface->DestroySession(LobbySessionName);
    }

    OnCreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::HandleCreateSessionComplete));

    FOnlineSessionSettings Settings;
    Settings.bIsLANMatch = false;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;
    Settings.bUseLobbiesIfAvailable = true;
    Settings.bUseLobbiesVoiceChatIfAvailable = true;
    Settings.bUsesPresence = true;
    Settings.bAllowJoinViaPresence = true;
    Settings.NumPublicConnections = PublicConnections;

    Settings.Set(SEARCH_LOBBIES, true, EOnlineDataAdvertisementType::ViaOnlineService);
    const FString ServerAddr = FString::Printf(TEXT("%s:%d"), *ServerIp, ServerPort);
    Settings.Set(FName("SERVER_ADDR"), ServerAddr, EOnlineDataAdvertisementType::ViaOnlineService);
    Settings.Set(FName("LOBBY_TAG"), FString("BIO"), EOnlineDataAdvertisementType::ViaOnlineService);

    SessionInterface->CreateSession(LocalUserNum, LobbySessionName, Settings);
}

void USessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteHandle);
    }

    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Error, TEXT("[Session] ✗ Lobby creation failed"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Session] ✓ Lobby created successfully"));

    if (SessionInterface.IsValid())
    {
        SessionInterface->StartSession(SessionName);
    }

    TravelToDedicatedFromLobby(SessionName);
}

void USessionSubsystem::FindGameSessions(int32 MaxResults, bool bIsLAN)
{
    UWorld* World = GetWorld();
    if (!World || IsRunningDedicatedServer())
    {
        return;
    }

    IOnlineIdentityPtr Identity = Online::GetIdentityInterface(World);
    if (!Identity.IsValid())
    {
        return;
    }

    const int32 LocalUserNum = 0;
    if (Identity->GetLoginStatus(LocalUserNum) != ELoginStatus::LoggedIn || !Identity->GetUniquePlayerId(LocalUserNum).IsValid())
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

    OnFindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionSubsystem::HandleFindSessionsComplete));

    if (!SessionInterface->FindSessions(LocalUserNum, SessionSearch.ToSharedRef()))
    {
        UE_LOG(LogTemp, Error, TEXT("[Session] FindSessions failed to start"));
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
    }
}

void USessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteHandle);
    }

    LastSessionInfos.Empty();

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
        {
            const auto& Result = SessionSearch->SearchResults[i];

            FSessionInfo Info;
            Info.SearchResultIndex = i;
            Info.PingInMs = Result.PingInMs;

            FString Name;
            if (!Result.Session.SessionSettings.Get(FName(TEXT("SESSION_NAME")), Name))
            {
                Name = FString::Printf(TEXT("Session_%d"), i);
            }
            Info.SessionName = Name;

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

        UE_LOG(LogTemp, Warning, TEXT("[Session] ✓ Found %d lobbies"), SessionSearch->SearchResults.Num());
    }

    OnSessionSearchUpdated.Broadcast(LastSessionInfos);
}

void USessionSubsystem::StartGameSession()
{
    UWorld* World = GetWorld();
    if (!World || !World->GetAuthGameMode())
    {
        return;
    }

    FString CurrentMap = World->GetMapName();
    if (CurrentMap.Contains(TEXT("TestSession")))
    {
        return;
    }

    FString TravelURL = TEXT("/Game/Level/TestSession?listen");
    UE_LOG(LogTemp, Warning, TEXT("[Session] ✓ Starting game session"));
    World->ServerTravel(TravelURL, false);
}

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
        return;
    }

    FString Addr;
    if (!Named->SessionSettings.Get(FName("SERVER_ADDR"), Addr) || Addr.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[Session] SERVER_ADDR missing"));
        return;
    }

    if (!Addr.Contains(TEXT(":")))
    {
        Addr = FString::Printf(TEXT("%s:%d"), *Addr, 7777);
    }

    PC->ClientTravel(Addr, ETravelType::TRAVEL_Absolute);
}

void USessionSubsystem::JoinFoundSession(int32 index)
{
    if (IsRunningDedicatedServer() || !SessionInterface.IsValid() || !SessionSearch.IsValid())
    {
        return;
    }

    if (SessionSearch->SearchResults.Num() <= 0 || index >= SessionSearch->SearchResults.Num())
    {
        return;
    }

    const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[index];

    OnJoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionSubsystem::HandleJoinSessionComplete));

    UWorld* World = GetWorld();
    if (!World || !World->GetFirstPlayerController())
    {
        return;
    }

    const int32 LocalUserNum = 0;
    const FName LobbySessionName = TEXT("LobbySession");

    if (!SessionInterface->JoinSession(LocalUserNum, LobbySessionName, Result))
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }
}

void USessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteHandle);
    }

    EJoinResultBP BPResult = EJoinResultBP::UnknownError;

    switch (Result)
    {
    case EOnJoinSessionCompleteResult::Success:
        BPResult = EJoinResultBP::Success;
        UE_LOG(LogTemp, Warning, TEXT("[Session] ✓ Successfully joined lobby"));
        break;
    case EOnJoinSessionCompleteResult::SessionIsFull:
        BPResult = EJoinResultBP::SessionIsFull;
        UE_LOG(LogTemp, Warning, TEXT("[Session] ✗ Lobby is full"));
        break;
    case EOnJoinSessionCompleteResult::SessionDoesNotExist:
        BPResult = EJoinResultBP::SessionDoesNotExist;
        UE_LOG(LogTemp, Warning, TEXT("[Session] ✗ Lobby does not exist"));
        break;
    case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
        BPResult = EJoinResultBP::CouldNotRetrieveAddress;
        UE_LOG(LogTemp, Warning, TEXT("[Session] ✗ Could not retrieve address"));
        break;
    case EOnJoinSessionCompleteResult::AlreadyInSession:
        BPResult = EJoinResultBP::AlreadyInSession;
        UE_LOG(LogTemp, Warning, TEXT("[Session] ✗ Already in session"));
        break;
    default:
        BPResult = EJoinResultBP::UnknownError;
        UE_LOG(LogTemp, Warning, TEXT("[Session] ✗ Unknown error"));
        break;
    }

    OnJoinSessionFinished.Broadcast(BPResult);

    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        const FName LobbySessionName = TEXT("LobbySession");
        TravelToDedicatedFromLobby(LobbySessionName);
    }
}