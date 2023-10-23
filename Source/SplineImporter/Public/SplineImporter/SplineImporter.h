// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#pragma once

#include "LogSplineImporter.h"

#include "Landscape.h"
#include "Components/SplineComponent.h" 

#pragma warning(disable: 4668)
#include "gdal.h"
#include "gdal_priv.h"
#include "gdal_utils.h"
#include <ogr_api.h>
#include <ogrsf_frmts.h>
#pragma warning(default: 4668)

#include "SplineImporter.generated.h"

#define LOCTEXT_NAMESPACE "FSplineImporterModule"

UENUM(BlueprintType)
enum ESourceKind: uint8 {
	Roads,
	Rivers,
	Buildings,
	OverpassShortQuery,
	OverpassQuery,
	LocalFile
};

UCLASS()
class SPLINEIMPORTER_API ASplineImporter : public AActor
{
	GENERATED_BODY()

public:
	ASplineImporter();

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (DisplayPriority = "-10")
	)
	TEnumAsByte<ESourceKind> SplinesSource = ESourceKind::Roads;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (DisplayPriority = "-5")
	)
	/* Check this only if `ActorToPlaceSplines` is a landscape and if you want to use landscape splines instead of normal spline components.
	 * For roads, it is recommended you use landscape splines (checked).
	 * For buildings, it is recommended you use normal spline components (unchecked). */
	bool bUseLandscapeSplines = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (DisplayPriority = "0")
	)
	AActor *ActorOrLandscapeToPlaceSplines = nullptr;

	/* Check this if you don't want to use import splines on the whole area of `ActorToPlaceSplines`.
	 * Then, set the `BoundingActor` to a cube or another rectangular actor covering the area that you want. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (DisplayPriority = "1")
	)
	bool bRestrictArea = false;

	/* Increasing this value (before generating splines) makes your landscape splines less curvy. This does not apply to regular splines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (EditCondition = "bUseLandscapeSplines", EditConditionHides, DisplayPriority = "1")
	)
	double LandscapeSplinesStraightness = 1;

	/* Use a cube or another rectangular actor to specify the area on which you want to import splines.
	 * Splines will be imported on `ActorToPlaceSplines`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (EditCondition = "bRestrictArea", EditConditionHides, DisplayPriority = "2")
	)
	AActor *BoundingActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (EditCondition = "SplinesSource == ESourceKind::LocalFile", EditConditionHides, DisplayPriority = "3")
	)
	FString LocalFile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (EditCondition = "SplinesSource == ESourceKind::OverpassQuery", EditConditionHides, DisplayPriority = "3")
	)
	FString OverpassQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Importer",
		meta = (EditCondition = "SplinesSource == ESourceKind::OverpassShortQuery", EditConditionHides, DisplayPriority = "3")
	)
	FString OverpassShortQuery;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spline Importer",
		meta = (DisplayPriority = "4")
	)
	void GenerateSplines();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Spline Importer",
		meta = (DisplayPriority = "5")
	)
	/* Toggle linear splines for regular splines. (For landscape splines, you can manually set the tangent length to 0 in the Segments Connections). */
	void ToggleLinear();

private:
	UPROPERTY()
	TArray<TObjectPtr<USplineComponent>> SplineComponents;

	GDALDataset* LoadGDALDatasetFromFile(FString File);
	void LoadGDALDataset(TFunction<void(GDALDataset*)> OnComplete);
	void LoadGDALDatasetFromQuery(FString Query, TFunction<void(GDALDataset*)> OnComplete);
	void LoadGDALDatasetFromShortQuery(FString ShortQuery, TFunction<void(GDALDataset*)> OnComplete);

	void GenerateLandscapeSplines(
		ALandscape *Landscape,
		FCollisionQueryParams CollisionQueryParams,
		TArray<TArray<OGRPoint>> &PointLists
	);

	void AddLandscapeSplinesPoints(
		ALandscape* Landscape,
		FCollisionQueryParams CollisionQueryParams,
		ULandscapeSplinesComponent* LandscapeSplinesComponent,
		TArray<OGRPoint> &PointList,
		TMap<FVector2D, ULandscapeSplineControlPoint*> &Points
	);

	void AddLandscapeSplines(
		ALandscape* Landscape,
		FCollisionQueryParams CollisionQueryParams,
		ULandscapeSplinesComponent* LandscapeSplinesComponent,
		TArray<OGRPoint> &PointList,
		TMap<FVector2D, ULandscapeSplineControlPoint*> &Points
	);

	void GenerateRegularSplines(
		AActor *Actor,
		FCollisionQueryParams CollisionQueryParams,
		TArray<TArray<OGRPoint>> &PointLists
	);

	void AddRegularSpline(
		AActor* Actor,
		FCollisionQueryParams CollisionQueryParams,
		TArray<OGRPoint> &PointList
	);
};

#undef LOCTEXT_NAMESPACE