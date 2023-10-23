// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#pragma once

#include "Coordinates/GlobalCoordinates.h"
#include "LandscapeCombinator/HMFetcher.h"

#include "ConsoleHelpers/ExternalTool.h"

#include "CoreMinimal.h"
#include "Landscape.h"

#include "LandscapeSpawner.generated.h"

#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

UENUM(BlueprintType)
enum class EHeightMapSourceKind : uint8
{
	Viewfinder15,
	Viewfinder3,
	Viewfinder1,
	SwissALTI_3D,
	USGS_OneThird,
	RGEALTI,
	LocalFile,
	LocalFolder,
	Litto3D_Guadeloupe,
	URL
};

UENUM(BlueprintType)
enum class EPresetHeightmap : uint8
{
	/* Viewfinder Panoramas 15, 15-A, 15-B, 15-C, 15-D, 15-E, 15-F, 15-G, 15-H, 15-I, 15-J, 15-K, 15-L, 15-M, 15-N, 15-O, 15-P, 15-Q, 15-R, 15-S, 15-T, 15-U, 15-V, 15-W, 15-X */
	WholeWorld,
	
	/* Viewfinder Panoramas 3, M30, M31, M32, L30, L31, L32, K30, K31, K32 */
	France,

	/* Viewfinder Panoramas 3, SF40 */
	Reunion,

	/* RGE ALTI, 503200, 569700, 6183000, 6227000, 4300, 4300 */
	PyreneesAriege,

	/* RGE ALTI, 988387, 990239, 6469448, 6470383 */
	PointeEchelle,
};

UCLASS(BlueprintType)
class LANDSCAPECOMBINATOR_API ALandscapeSpawner : public AActor
{
	GENERATED_BODY()

public:
	ALandscapeSpawner();
	
	// FIXME: add presets
	//UPROPERTY(
	//	EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
	//	meta = (DisplayPriority = "-10")
	//)
	//EPresetHeightmap PresetHeightmap;

		/***********************
	  *  Heightmap Source  *
	  **********************/
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (DisplayPriority = "1")
	)
	/* Please select the source from which we download the heightmaps. */
	EHeightMapSourceKind HeightMapSourceKind;


	/***************************
	  *  Viewfinder Panoramas  *
	  **************************/
		
	UPROPERTY(
		VisibleAnywhere, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Viewfinder15", EditConditionHides, DisplayPriority = "3")
	)
	FString Viewfinder15_Help = "Enter the comma-separated list of rectangles (e.g. 15-A, 15-B, 15-G, 15-H) from http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org15.htm";
		
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Viewfinder15", EditConditionHides, DisplayPriority = "4")
	)
	FString Viewfinder15_TilesString;
		
	UPROPERTY(
		VisibleAnywhere, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Viewfinder3", EditConditionHides, DisplayPriority = "3")
	)
	FString Viewfinder3_Help = "Enter the comma-separated list of rectangles (e.g. M31, M32) from http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org3.htm";
		
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Viewfinder3", EditConditionHides, DisplayPriority = "4")
	)
	FString Viewfinder3_TilesString;
		
	UPROPERTY(
		VisibleAnywhere, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Viewfinder1", EditConditionHides, DisplayPriority = "3")
	)
	FString Viewfinder1_Help = "Enter the comma-separated list of rectangles (e.g. M31, M32) from http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org1.htm";
		
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Viewfinder1", EditConditionHides, DisplayPriority = "4")
	)
	FString Viewfinder1_TilesString;

	
	/******************
	  *  SwissALTI3D  *
	  *****************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::SwissALTI_3D", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter C:\Path\To\ListOfLinks.csv (see README for more details) */
	FString SwissALTI3DListOfLinks;

	
	/*********************
	  *  USGS One Third  *
	  ********************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::USGS_OneThird", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter C:\Path\To\ListOfLinks.txt (see README for more details) */
	FString USGS_OneThirdListOfLinks;
	

	
	/*******************
	  *  Local Folder  *
	  ******************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Folder", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter C:\Path\To\Folder\ containing heightmaps correctly following the _x0_y0 convention */
	FString Folder;

	

	/*************
	  *  Litto3D *
	  ************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Litto3D_Guadeloupe", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter C:\Path\To\Folder\ containing 7z files downloaded from https://diffusion.shom.fr/multiproduct/product/configure/id/108 */
	FString Litto3D_Folder;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Litto3D_Guadeloupe", EditConditionHides, DisplayPriority = "4")
	)
	/* Tick this if you prefer to use the less precise 5m data instead of 1m data */
	bool bUse5mData = false;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::Litto3D_Guadeloupe", EditConditionHides, DisplayPriority = "4")
	)
	/* Tick this if the files have already been extracted once. Keep it checked if unsure. */
	bool bSkipExtraction = false;



	/***************
	  *  RGE ALTI  *
	  **************/
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter the minimum longitude of the bounding box in EPSG 2154 coordinates (left coordinate) */
	double RGEALTIMinLong;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI", EditConditionHides, DisplayPriority = "4")
	)
	/* Enter the maximum longitude of the bounding box in EPSG 2154 coordinates (right coordinate) */
	double RGEALTIMaxLong;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI", EditConditionHides, DisplayPriority = "5")
	)
	/* Enter the minimum latitude of the bounding box in EPSG 2154 coordinates (bottom coordinate) */
	double RGEALTIMinLat;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI", EditConditionHides, DisplayPriority = "6")
	)
	/* Enter the maximum latitude of the bounding box in EPSG 2154 coordinates (top coordinate) */
	double RGEALTIMaxLat;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI", EditConditionHides, DisplayPriority = "7")
	)
	/* When set to true, you can specify the width and height of the desired heightmap,
	 * so that the RGE ALTI web API resizes your image before download.
	 * Maximum size for this API is 10,000 pixels. */
	bool bResizeRGEAltiUsingWebAPI;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI && bResizeRGEALTIUsingWebAPI", EditConditionHides, DisplayPriority = "8")
	)
	/* Enter desired width for the downloaded heightmap from RGE ALTI web API */
	int RGEALTIWidth;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::RGEALTI && bResizeRGEALTIUsingWebAPI", EditConditionHides, DisplayPriority = "9")
	)
	/* Enter desired height for the downloaded heightmap from RGE ALTI web API */
	int RGEALTIHeight;

	

	/****************
	  * Local Files *
	  ***************/	
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::LocalFile", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter your C:\\Path\\To\\MyHeightmap.tif in GeoTIFF format */
	FString LocalFilePath;

	

	/********
	  * URL *
	  *******/	
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Source",
		meta = (EditCondition = "HeightMapSourceKind == EHeightMapSourceKind::LocalFile", EditConditionHides, DisplayPriority = "3")
	)
	/* Enter URL to a heightmap in GeoTIFF format */
	FString URL;


	/*********************
	  * General Settings *
	  ********************/	


	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|General",
		meta = (DisplayPriority = "0")
	)
	/* Label of the landscape to create. */
	FString LandscapeLabel;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|General",
		meta = (DisplayPriority = "1")
	)
	/* The scale in the Z-axis of your heightmap, ZScale = 1 corresponds to real-world size. */
	double ZScale = 1;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|General",
		meta = (DisplayPriority = "2")
	)
	/* When the landscape components do not exactly match with the total size of the heightmaps,
	 * Unreal Engine adds some padding at the border. Check this option if you are willing to
	 * drop some data at the border in order to make your heightmaps exactly match the landscape
	 * components. */
	bool bDropData = true;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|General",
		meta = (DisplayPriority = "3")
	)
	/* If this is checked, then the LevelCoordinates world origin and EPSG will be set to the center of this landscape.
	 * WARNING: If you have more than one landscape, you must use this option only on one landscape,
	 * otherwise each landscape will be aligned with (0, 0), instead of being well aligned with respect to the other landscapes. */
	bool bSetLevelCoordinatesWorldOrigin = true;



	/*****************
	 * Preprocessing *
	 *****************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Preprocessing",
		meta = (DisplayPriority = "10")
	)
	/* Tick this if you want to run an external binary to prepare the heightmaps right after fetching them. */
	bool bPreprocess = false;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Preprocessing",
		meta = (EditCondition = "bPreprocess", EditConditionHides, DisplayPriority = "11")
	)
	TObjectPtr<UExternalTool> PreprocessingTool;


	/****************
	 * Reprojection *
	 ****************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Reprojection",
		meta = (EditCondition = "!bSetLevelCoordinatesWorldOrigin", EditConditionHides, DisplayPriority = "30")
	)
	/* Keep this checked if the coordinate system of the heightmap data is different than the coordinate system of your level (LevelCoordinates).
	 * This triggers a reprojection of your heightmap data in the level coordinate system. */
	bool bRequiresReprojection = true;


	/******************
	 * Convert To PNG *
	 *****************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|ConversionToPNG",
		meta = (DisplayPriority = "30")
	)
	/* Untick this if your heightmaps are already in PNG format. */
	bool bConvertToPNG = true;


	/**************
	 * Resolution *
	 **************/

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Resolution",
		meta = (DisplayPriority = "20")
	)
	/* Tick this if you wish to scale up or down the resolution of your heightmaps. */
	bool bChangeResolution = false;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "LandscapeSpawner|Resolution",
		meta = (EditCondition = "bChangeResolution", EditConditionHides, DisplayPriority = "21")
	)
	/* When different than 100%, your heightmap resolution will be scaled up or down using GDAL. */
	int PrecisionPercent = 100;


	
	/**************
	  * Actions *
	  *************/
	
	/* Spawn the Landscape. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LandscapeSpawner",
		meta = (DisplayPriority = "10")
	)
	void SpawnLandscape();
	
	/* This deletes all the heightmaps, included downloaded files. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LandscapeSpawner",
		meta = (DisplayPriority = "11")
	)
	void DeleteAllHeightmaps();

	/* This preserves downloaded files. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LandscapeSpawner",
		meta = (DisplayPriority = "12")
	)
	void DeleteAllProcessedHeightmaps();

	

private:
	HMFetcher* CreateInitialFetcher();
	HMFetcher* CreateFetcher(HMFetcher *InitialFetcher);

	// After fetching the heightmaps, this holds the min max altitudes as computer by GDAL right before converting to 16-bit PNG
	FVector2D Altitudes;
};

#undef LOCTEXT_NAMESPACE