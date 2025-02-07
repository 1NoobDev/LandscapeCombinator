// Copyright 2023 LandscapeCombinator. All Rights Reserved.

using UnrealBuildTool;

public class HeightmapModifier : ModuleRules
{
	public HeightmapModifier(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_2;

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
				// Unreal Dependencies
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Foliage",
				"Engine",
				"Landscape",
				"LandscapeEditor",
				
				// Other Dependencies
				"GDALInterface",
				"LandscapeUtils",
				"ConsoleHelpers"
			}
		);
	}
}
