// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BioProtocol : ModuleRules
{
	public BioProtocol(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "Slate", "SlateCore", "OnlineSubsystemEOS", "OnlineSubsystemSteam", "OnlineSubsystem" ,"VoiceChat", "EOSVoiceChat", "HTTP", "Json", "JsonUtilities"});

		PrivateDependencyModuleNames.AddRange(new string[] {  });


        // 이 모듈의 Public 헤더 경로 추가 (예: Source/BioProtocol/Public)
        PublicIncludePaths.AddRange(new string[] {
            "BioProtocol/Public"
        });


        // 이 모듈의 Private 헤더 경로 추가 (예: Source/BioProtocol/Private)
        PrivateIncludePaths.AddRange(new string[] {
            "BioProtocol/Private"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
