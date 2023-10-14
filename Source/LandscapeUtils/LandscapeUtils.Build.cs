// Copyright 2023 LandscapeCombinator. All Rights Reserved.

using UnrealBuildTool;

public class LandscapeUtils : ModuleRules
{
	public LandscapeUtils(ReadOnlyTargetRules Target) : base(Target)
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
				"Foliage",
				"Landscape",
				"LandscapeEditor",
				"UnrealEd",
				"Slate",
				"EditorFramework"
			}
		);
	}
}
