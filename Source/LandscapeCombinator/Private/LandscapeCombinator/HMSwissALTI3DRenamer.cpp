// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "LandscapeCombinator/HMSwissALTI3DRenamer.h"

#include "Internationalization/Regex.h"

int HMSwissALTI3DRenamer::TileToX(FString Tile) const
{
	FRegexPattern Pattern(TEXT("swissalti3d_\\d+_(\\d+)-\\d+/"));
	FRegexMatcher Matcher(Pattern, Tile);
	Matcher.FindNext();
	FString X = Matcher.GetCaptureGroup(1);
	return FCString::Atoi(*X);
}

int HMSwissALTI3DRenamer::TileToY(FString Tile) const
{
	FRegexPattern Pattern(TEXT("swissalti3d_\\d+_\\d+-(\\d+)/"));
	FRegexMatcher Matcher(Pattern, Tile);
	Matcher.FindNext();
	FString Y = Matcher.GetCaptureGroup(1);
	return - FCString::Atoi(*Y);
}