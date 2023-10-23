// Copyright 2023 LandscapeCombinator. All Rights Reserved.

using UnrealBuildTool;

public class SplineImporter : ModuleRules
{
	public SplineImporter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// Unreal Engine Dependencies
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Unreal Engine Dependencies
				"CoreUObject",
				"Engine",
				"PCG",
				"Foliage",
				"Landscape",
				"LandscapeEditor",
				"UnrealEd",
				"HTTP",

				// Other Dependencies
				"LandscapeUtils",
				"Coordinates",
				"GDALInterface",
				"FileDownloader"
			}
		);
	}
}
