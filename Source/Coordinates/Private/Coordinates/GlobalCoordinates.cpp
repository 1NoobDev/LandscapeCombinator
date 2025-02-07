// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "Coordinates/GlobalCoordinates.h"

#include "LandscapeUtils/LandscapeUtils.h"

#define LOCTEXT_NAMESPACE "FCoordinatesModule"

void UGlobalCoordinates::GetUnrealCoordinatesFromCRS(double Longitude, double Latitude, FVector2D &XY)
{
	XY[0] = (Longitude - WorldOriginLong) * CmPerLongUnit;
	XY[1] = (Latitude - WorldOriginLat) * CmPerLatUnit;
}

OGRCoordinateTransformation* UGlobalCoordinates::GetCRSTransformer(FString FromCRS)
{
	OGRSpatialReference InRs, OutRs;
	if (!GDALInterface::SetCRSFromUserInput(InRs, FromCRS) || !GDALInterface::SetCRSFromUserInput(OutRs, CRS))
	{ 
		return nullptr;
	}
	
	return OGRCreateCoordinateTransformation(&InRs, &OutRs);
}

bool UGlobalCoordinates::GetUnrealCoordinatesFromCRS(double Longitude, double Latitude, FString FromCRS, FVector2D &XY)
{
	double ConvertedLongitude = Longitude;
	double ConvertedLatitude = Latitude;
	
	OGRSpatialReference InRs, OutRs;
	if (
		!GDALInterface::SetCRSFromUserInput(InRs, FromCRS) ||
		!GDALInterface::SetCRSFromUserInput(OutRs, CRS) ||
		!OGRCreateCoordinateTransformation(&InRs, &OutRs)->Transform(1, &ConvertedLongitude, &ConvertedLatitude)
	)
	{	
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("GetUnrealCoordinatesFromCRS", "Internal error while transforming coordinates.")
		);
		return false;
	}

	XY[0] = (ConvertedLongitude - WorldOriginLong) * CmPerLongUnit;
	XY[1] = (ConvertedLatitude - WorldOriginLat) * CmPerLatUnit;

	return true;
}


void UGlobalCoordinates::GetCRSCoordinatesFromUnrealLocation(FVector2D Location, FVector2D& OutCoordinates)
{
	// in Global CRS
	OutCoordinates[0] = Location.X / CmPerLongUnit + WorldOriginLong;
	OutCoordinates[1] = Location.Y / CmPerLatUnit + WorldOriginLat;
}

bool UGlobalCoordinates::GetCRSCoordinatesFromUnrealLocation(FVector2D Location, FString ToCRS, FVector2D& OutCoordinates)
{
	// in Global CRS
	OutCoordinates[0] = Location.X / CmPerLongUnit + WorldOriginLong;
	OutCoordinates[1] = Location.Y / CmPerLatUnit + WorldOriginLat;
	
	// convert to ToCRS
	OGRSpatialReference InRs, OutRs;
	if (
		!GDALInterface::SetCRSFromUserInput(InRs, CRS) ||
		!GDALInterface::SetCRSFromUserInput(OutRs, ToCRS) ||
		!OGRCreateCoordinateTransformation(&InRs, &OutRs)->Transform(1, &OutCoordinates[0], &OutCoordinates[1])
	)
	{	
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("GetCRSCoordinatesFromUnrealLocation", "Internal error while transforming coordinates.")
		);
		return false;
	}

	return true;
}

void UGlobalCoordinates::GetCRSCoordinatesFromUnrealLocations(FVector4d Locations, FVector4d& OutCoordinates)
{
	// in Global CRS
	double xs[2] = { Locations[0] / CmPerLongUnit + WorldOriginLong,  Locations[1] / CmPerLongUnit + WorldOriginLong  };
	double ys[2] = { Locations[3] / CmPerLatUnit + WorldOriginLat, Locations[2] / CmPerLatUnit + WorldOriginLat };
	
	OutCoordinates[0] = xs[0];
	OutCoordinates[1] = xs[1];
	OutCoordinates[2] = ys[1];
	OutCoordinates[3] = ys[0];
}

bool UGlobalCoordinates::GetCRSCoordinatesFromUnrealLocations(FVector4d Locations, FString ToCRS, FVector4d& OutCoordinates)
{
	// in Global CRS
	double xs[2] = { Locations[0] / CmPerLongUnit + WorldOriginLong,  Locations[1] / CmPerLongUnit + WorldOriginLong  };
	double ys[2] = { Locations[3] / CmPerLatUnit + WorldOriginLat, Locations[2] / CmPerLatUnit + WorldOriginLat };
	
	// convert to ToCRS
	OGRSpatialReference InRs, OutRs;
	if (
		!GDALInterface::SetCRSFromUserInput(InRs, CRS) ||
		!GDALInterface::SetCRSFromUserInput(OutRs, ToCRS) ||
		!OGRCreateCoordinateTransformation(&InRs, &OutRs)->Transform(2, xs, ys)
	)
	{	
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("GetCRSCoordinatesFromUnrealLocation", "Internal error while transforming coordinates.")
		);
		return false;
	}
	
	OutCoordinates[0] = xs[0];
	OutCoordinates[1] = xs[1];
	OutCoordinates[2] = ys[1];
	OutCoordinates[3] = ys[0];

	return true;
}

bool UGlobalCoordinates::GetCRSCoordinatesFromFBox(FBox Box, FString ToCRS, FVector4d& OutCoordinates)
{
	return GetCRSCoordinatesFromOriginExtent(Box.GetCenter(), Box.GetExtent(), ToCRS, OutCoordinates);
}

bool UGlobalCoordinates::GetCRSCoordinatesFromOriginExtent(FVector Origin, FVector Extent, FString ToCRS, FVector4d& OutCoordinates)
{
	FVector4d Locations;
	Locations[0] = Origin.X - Extent.X;
	Locations[1] = Origin.X + Extent.X;
	Locations[2] = Origin.Y + Extent.Y;
	Locations[3] = Origin.Y - Extent.Y;

	return GetCRSCoordinatesFromUnrealLocations(Locations, ToCRS, OutCoordinates);
}


bool UGlobalCoordinates::GetLandscapeBounds(ALandscape *Landscape, FString ToCRS, FVector4d &OutCoordinates)
{
	FVector2D MinMaxX, MinMaxY, UnusedMinMaxZ;
	if (!LandscapeUtils::GetLandscapeBounds(Landscape, MinMaxX, MinMaxY, UnusedMinMaxZ))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("LoadGDALDatasetFromShortQuery3", "Could not compute bounds of Landscape {0}"),
			FText::FromString(Landscape->GetActorLabel())
		));
		return false;
	}

	FVector4d Locations;
	Locations[0] = MinMaxX[0];
	Locations[1] = MinMaxX[1];
	Locations[2] = MinMaxY[1];
	Locations[3] = MinMaxY[0];
	if (!GetCRSCoordinatesFromUnrealLocations(Locations, ToCRS, OutCoordinates))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("LoadGDALDatasetFromShortQuery3", "Could not compute coordinates of Landscape {0}"),
			FText::FromString(Landscape->GetActorLabel())
		));
		return false;
	}

	return true;
}


#undef LOCTEXT_NAMESPACE