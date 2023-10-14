// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "LandscapeCombinator/HMRGEAlti.h"

#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

HMURL* HMRGEALTI::RGEALTI(FString LandscapeLabel, double MinLong, double MaxLong, double MinLat, double MaxLat, bool bOverrideWidthAndHeight, int Width0, int Height0)
{
	int Width = Width0;
	int Height = Height0;

	if (!bOverrideWidthAndHeight)
	{
		Width =  MaxLong - MinLong;
		Height = MaxLat - MinLat;
	}

	if (Width <= 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("RGEALTIInitErrorNegativeWidth", "The width of {0} is not positive."),
			FText::FromString(LandscapeLabel)
		));

		return nullptr;
	}

	if (Width > 10000)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("RGEALTIInitErrorLargeWidth", "The width of {0} is higher than 10000px, which is not supported by RGE ALTI 1m WMS."),
			FText::FromString(LandscapeLabel)
		));

		return nullptr;
	}

	if (Height <= 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("RGEALTIInitErrorNegativeHeight", "The height of {0} is not positive."),
			FText::FromString(LandscapeLabel)
		));

		return nullptr;
	}

	if (Height > 10000)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("RGEALTIInitErrorLargeHeight", "The height of {0} is higher than 10000px, which is not supported by RGE ALTI 1m WMS."),
			FText::FromString(LandscapeLabel)
		));

		return nullptr;
	}

	return new HMURL(
		FString::Format(
			TEXT("https://wxs.ign.fr/altimetrie/geoportail/r/wms?LAYERS=RGEALTI-MNT_PYR-ZIP_FXX_LAMB93_WMS&FORMAT=image/geotiff&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&CRS=EPSG:2154&BBOX={0},{1},{2},{3}&WIDTH={4}&HEIGHT={5}&STYLES="),
			{ MinLong, MinLat, MaxLong, MaxLat, Width, Height }
		),
		LandscapeLabel
	);
}

#undef LOCTEXT_NAMESPACE