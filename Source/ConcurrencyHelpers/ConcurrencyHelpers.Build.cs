// Copyright 2023 LandscapeCombinator. All Rights Reserved.

using UnrealBuildTool;

public class ConcurrencyHelpers : ModuleRules
{
	public ConcurrencyHelpers(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Unreal Dependencies
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"HTTP"
			}
		);
	}
}
