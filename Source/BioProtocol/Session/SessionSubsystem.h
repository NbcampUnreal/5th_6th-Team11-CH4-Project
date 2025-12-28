#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString SessionName;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxPlayers = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 PingInMs = 0;

    // 어떤 SearchResults 인덱스인지
    int32 SearchResultIndex = INDEX_NONE;
};

UENUM(BlueprintType)
enum class EJoinResultBP : uint8
{
    Success,
    SessionIsFull,
    SessionDoesNotExist,
    CouldNotRetrieveAddress,
    AlreadyInSession,
    UnknownError
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionSearchUpdated, const TArray<FSessionInfo>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionFinished, EJoinResultBP, Result);

UCLASS()
class BIOPROTOCOL_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    // 세션 생성
    UFUNCTION(BlueprintCallable, Category = "Session")
    void CreateGameSession(int32 PublicConnections = 4, bool bIsLAN = true);

    // 세션 검색
    UFUNCTION(BlueprintCallable, Category = "Session")
    void FindGameSessions(int32 MaxResults = 100, bool bIsLAN = true);

    // 세션 참가 추후에 인덱스로 들어가게 수정
    UFUNCTION(BlueprintCallable, Category = "Session")
    void JoinFoundSession(int32 index);

    UFUNCTION(BlueprintCallable, Category = "Session")
    void StartGameSession();

    UPROPERTY(BlueprintAssignable)
    FOnSessionSearchUpdated OnSessionSearchUpdated;

    UPROPERTY(BlueprintAssignable)
    FOnJoinSessionFinished OnJoinSessionFinished;

    UFUNCTION(BlueprintCallable)
    const TArray<FSessionInfo>& GetLastSessionInfos() const { return LastSessionInfos; }


protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    IOnlineSessionPtr GetSessionInterface() const;

private:
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    TArray<FSessionInfo> LastSessionInfos;

    // 델리게이트 핸들
    FDelegateHandle OnCreateSessionCompleteHandle;
    FDelegateHandle OnFindSessionsCompleteHandle;
    FDelegateHandle OnJoinSessionCompleteHandle;

    // 콜백
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
