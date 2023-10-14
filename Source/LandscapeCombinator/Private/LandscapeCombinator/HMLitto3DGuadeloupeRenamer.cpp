// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "LandscapeCombinator/HMLitto3DGuadeloupeRenamer.h"

#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

int HMLitto3DGuadeloupeRenamer::TileToX(FString Tile) const
{
	FRegexPattern Pattern(TEXT("LITTO3D_GUA_(\\d+)_\\d+_MNT"));
	FRegexMatcher Matcher(Pattern, Tile);
	Matcher.FindNext();
	FString X = Matcher.GetCaptureGroup(1);
	return FCString::Atoi(*X);
}

int HMLitto3DGuadeloupeRenamer::TileToY(FString Tile) const
{
	FRegexPattern Pattern(TEXT("LITTO3D_GUA_\\d+_(\\d+)_MNT"));
	FRegexMatcher Matcher(Pattern, Tile);
	Matcher.FindNext();
	FString Y = Matcher.GetCaptureGroup(1);
	return - FCString::Atoi(*Y);
}

#undef LOCTEXT_NAMESPACE
