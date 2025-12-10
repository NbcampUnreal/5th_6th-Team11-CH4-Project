// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class BIOPROTOCOL_API USessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
    // 세션 생성
    void CreateGameSession(int32 PublicConnections = 4, bool bIsLAN = true);

    // 세션 검색
    void FindGameSessions(int32 MaxResults = 100, bool bIsLAN = true);

    // 세션 참가 (가장 첫 번째 결과에 조인하는 단순 버전)
    void JoinFoundSession();

protected:
    // Subsystem 라이프사이클
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    IOnlineSessionPtr GetSessionInterface() const;

private:
    // OnlineSubsystem & Session 인터페이스 포인터
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    // 델리게이트 핸들
    FDelegateHandle OnCreateSessionCompleteHandle;
    FDelegateHandle OnFindSessionsCompleteHandle;
    FDelegateHandle OnJoinSessionCompleteHandle;

    // 콜백
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
};
