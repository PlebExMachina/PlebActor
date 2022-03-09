// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PlebActors : ModuleRules
{
	public PlebActors(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
