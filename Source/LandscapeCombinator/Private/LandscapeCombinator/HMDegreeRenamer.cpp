// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "LandscapeCombinator/HMDegreeRenamer.h"

#include "Internationalization/Regex.h"

int HMDegreeRenamer::TileToX(FString Tile) const
{
	FRegexPattern Pattern(TEXT("([WEwe])(\\d\\d\\d)"));
	FRegexMatcher Matcher(Pattern, Tile);
	Matcher.FindNext();
	FString Direction = Matcher.GetCaptureGroup(1);
	FString Degrees = Matcher.GetCaptureGroup(2);
	if (Direction == "W" || Direction == "w")
		return 180 - FCString::Atoi(*Degrees);
	else
		return 180 + FCString::Atoi(*Degrees);
}

int HMDegreeRenamer::TileToY(FString Tile) const
{
	FRegexPattern Pattern(TEXT("([NSns])(\\d\\d)"));
	FRegexMatcher Matcher(Pattern, Tile);
	Matcher.FindNext();
	FString Direction = Matcher.GetCaptureGroup(1);
	FString Degrees = Matcher.GetCaptureGroup(2);
	if (Direction == "N" || Direction == "n")
		return 83 - FCString::Atoi(*Degrees);
	else
		return 83 + FCString::Atoi(*Degrees);
}

