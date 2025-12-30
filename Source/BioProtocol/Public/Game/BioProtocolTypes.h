// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EBioPlayerRole : uint8
{
	None,
	Staff,      // 감염된 직원
	Cleaner     // 청소부 안드로이드 (AI)
};

UENUM(BlueprintType)
enum class EBioPlayerStatus : uint8
{
	Alive,      // 생존
	Jailed,     // 격리실 감금
	Dead        // 관전 모드
};

UENUM(BlueprintType)
enum class EBioGamePhase : uint8
{
	Day,        // 업무 및 파밍
	Night,      // 퍼지 타임
	Evacuate,   // 탈출 단계
	End         // 결과 화면
};