// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#pragma once

#include "HMInterfaceTiles.h"

#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

class HMViewFinder : public HMInterfaceTiles
{
protected:
	TArray<FString> MegaTiles;
	FString BaseURL;

	bool Initialize() override;
	int TileToX(FString Tile) const override;
	int TileToY(FString Tile) const override;
	
	bool GetCoordinatesSpatialReference(OGRSpatialReference &InRs) const override;
	bool GetDataSpatialReference(OGRSpatialReference &InRs) const override;

public:
	HMViewFinder(FString LandscapeLabel0, const FText &KindText0, FString Descr0, int Precision0);
	FReply DownloadHeightMapsImpl(TFunction<void(bool)> OnComplete) override;
};

#undef LOCTEXT_NAMESPACE