// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ComboBoxString.h"
#include "Blueprint/UserWidget.h" 

#include "WMSProvider.generated.h"

#define LOCTEXT_NAMESPACE "FImageDownloaderModule"

USTRUCT()
struct FWMSProvider
{
	GENERATED_BODY()

public:
	void SetFromURL(FString URL, TArray<FString> ExcludeCRS, TFunction<bool(FString)> NameFilter, TFunction<void(bool)> OnComplete);
	bool LoadFromFile(TArray<FString> ExcludeCRS, TFunction<bool(FString)> NameFilter);
	
	bool CreateURL(
		int Width, int Height, FString Name, FString CRS,
		double MinAllowedLong, double MaxAllowedLong, double MinAllowedLat, double MaxAllowedLat,
		double MinLong, double MaxLong, double MinLat, double MaxLat,
		FString &URL, bool &bGeoTiff, FString &FileExt
	);

	UPROPERTY();
	int MaxWidth;

	UPROPERTY();
	int MaxHeight;

	UPROPERTY();
	FString GetMapURL;

	UPROPERTY();
	FString CapabilitiesURL;

	UPROPERTY();
	FString CapabilitiesFile;

	UPROPERTY();
	FString CapabilitiesContent;

	UPROPERTY();
	TArray<FString> Names;

	UPROPERTY();
	TArray<FString> Titles;

	UPROPERTY();
	TArray<FString> Abstracts;

	UPROPERTY();
	TArray<FString> CRSs;

	UPROPERTY();
	TArray<double> MinXs;

	UPROPERTY();
	TArray<double> MinYs;

	UPROPERTY();
	TArray<double> MaxXs;

	UPROPERTY();
	TArray<double> MaxYs;

private:
	FString FindGetMapURL();
	void FindAndSetMaxSize();
};

#undef LOCTEXT_NAMESPACE