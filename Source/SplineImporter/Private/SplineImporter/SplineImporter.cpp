// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "SplineImporter/SplineImporter.h"
#include "SplineImporter/LogSplineImporter.h"
#include "SplineImporter/Overpass.h"
#include "FileDownloader/Download.h"
#include "LandscapeUtils/LandscapeUtils.h"
#include "Coordinates/LevelCoordinates.h"
#include "GDALInterface/GDALInterface.h"


#include "EditorSupportDelegates.h"
#include "LandscapeStreamingProxy.h"
#include "LandscapeSplineControlPoint.h" 
#include "LandscapeSplinesComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWindow.h"
#include "Misc/CString.h"
#include "Logging/StructuredLog.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Async/Async.h"
#include "Misc/ScopedSlowTask.h" 

#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

ASplineImporter::ASplineImporter()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent")));
	RootComponent->SetMobility(EComponentMobility::Static);
}

void ASplineImporter::GenerateSplines()
{
	if (!ActorOrLandscapeToPlaceSplines)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ASplineImporter::GenerateSplines::1", "Please select an actor on which to place your splines.")
		);
		return;
	}

	FText IntroMessage;
	ALandscape *Landscape = nullptr;
	FString ActorOrLandscapeToPlaceSplinesLabel = ActorOrLandscapeToPlaceSplines->GetActorLabel();

	if (bUseLandscapeSplines)
	{
		if (!ActorOrLandscapeToPlaceSplines->IsA<ALandscape>())
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(
					LOCTEXT("ASplineImporter::GenerateSplines::2", "Actor {0} is not a landscape"),
					FText::FromString(ActorOrLandscapeToPlaceSplinesLabel)
				)
			);
			return;
		}
		Landscape = Cast<ALandscape>(ActorOrLandscapeToPlaceSplines);

		IntroMessage = FText::Format(
			LOCTEXT("ASplineImporter::GenerateSplines::Intro1",
				"Landscape splines will now be added to Landscape {0}. You can monitor the progress in the Output Log. "
				"After the splines are added, you must go to\n"
				"Landscape Mode > Manage > Splines\n"
				"to manage the splines as usual. By selecting all control points or all segments, and then going to their Details panel, you can choose "
				"to paint a landscape layer for the roads, or you can add spline meshes to form your roads.\n"	
			),
			FText::FromString(ActorOrLandscapeToPlaceSplinesLabel)
		);
	}
	else
	{
		IntroMessage = FText::Format(
			LOCTEXT("ASplineImporter::GenerateSplines::Intro2",
				"Splines will now be added to {0}. You can monitor the progress in the Output Log. "
				"For splines that represent buildings perimeters, you can use the plugin's class BuildingsFromSplines to create buildings automatically."
			),
			FText::FromString(ActorOrLandscapeToPlaceSplinesLabel)
		);
	}
	
	EAppReturnType::Type UserResponse = FMessageDialog::Open(EAppMsgType::OkCancel, IntroMessage);
	if (UserResponse == EAppReturnType::Cancel) 
	{
		UE_LOG(LogSplineImporter, Log, TEXT("User cancelled adding landscape splines."));
		return;
	}

	LoadGDALDataset([this, Landscape](GDALDataset* Dataset) {
		if (Dataset)
		{
			TArray<TArray<OGRPoint>> PointLists = GDALInterface::GetPointLists(Dataset);
			GDALClose(Dataset);
			FCollisionQueryParams CollisionQueryParams = LandscapeUtils::CustomCollisionQueryParams(ActorOrLandscapeToPlaceSplines);

			if (bUseLandscapeSplines)
			{
				GenerateLandscapeSplines(Landscape, CollisionQueryParams, PointLists);
			}
			else
			{
				GenerateRegularSplines(ActorOrLandscapeToPlaceSplines, CollisionQueryParams, PointLists);
			}
		}
	});
}

GDALDataset* ASplineImporter::LoadGDALDatasetFromFile(FString File)
{
	GDALDataset* Dataset = (GDALDataset*) GDALOpenEx(TCHAR_TO_UTF8(*File), GDAL_OF_VECTOR, NULL, NULL, NULL);

	if (!Dataset)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("LandscapeNotFound", "Could not load vector file '{0}'."),
				FText::FromString(File)
			)
		);
	}

	return Dataset;
}

void ASplineImporter::LoadGDALDatasetFromQuery(FString Query, TFunction<void(GDALDataset*)> OnComplete)
{
	UE_LOG(LogSplineImporter, Log, TEXT("Adding roads with Overpass query: '%s'"), *Query);
	UE_LOG(LogSplineImporter, Log, TEXT("Decoded URL: '%s'"), *(FGenericPlatformHttp::UrlDecode(Query)));
	FString IntermediateDir = FPaths::ConvertRelativePathToFull(FPaths::EngineIntermediateDir());
	FString LandscapeCombinatorDir = FPaths::Combine(IntermediateDir, "LandscapeCombinator");
	FString DownloadDir = FPaths::Combine(LandscapeCombinatorDir, "Download");
	FString XmlFilePath = FPaths::Combine(DownloadDir, FString::Format(TEXT("overpass_query_{0}.xml"), { FTextLocalizationResource::HashString(Query) }));

	Download::FromURL(Query, XmlFilePath,
		[=, this](bool bWasSuccessful) {
			if (bWasSuccessful && OnComplete)
			{
				// working on splines only works in GameThread
				AsyncTask(ENamedThreads::GameThread, [=, this]() {
					OnComplete(LoadGDALDatasetFromFile(XmlFilePath));
				});
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
					LOCTEXT("GetSpatialReferenceError", "Unable to get the result for the Overpass query: {0}."),
					FText::FromString(Query)
				));
			}
			return;
		}
	);
}

void ASplineImporter::LoadGDALDatasetFromShortQuery(FString ShortQuery, TFunction<void(GDALDataset*)> OnComplete)
{
	UE_LOG(LogSplineImporter, Log, TEXT("Adding roads with short Overpass query: '%s'"), *ShortQuery);
	
	FVector4d Coordinates;
	if (bRestrictArea)
	{
		if (!BoundingActor)
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("LoadGDALDatasetFromShortQuery", "Please set BoundingActor to a valid actor, or untick the RestrictArea option")
			);
			if (OnComplete) OnComplete(nullptr);
			return;
		}
		FVector Origin, BoxExtent;
		BoundingActor->GetActorBounds(true, Origin, BoxExtent);
		if (!ALevelCoordinates::GetEPSGCoordinatesFromOriginExtent(BoundingActor->GetWorld(), Origin, BoxExtent, 4326, Coordinates))
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("LoadGDALDatasetFromShortQuery2", "Internal error while reading coordinates. Make sure that your level coordinates are valid.")
			);
			if (OnComplete) OnComplete(nullptr);
			return;
		}
	}
	else if (!ActorOrLandscapeToPlaceSplines->IsA<ALandscape>())
	{
		FVector Origin, BoxExtent;
		ActorOrLandscapeToPlaceSplines->GetActorBounds(true, Origin, BoxExtent);
		if (!ALevelCoordinates::GetEPSGCoordinatesFromOriginExtent(ActorOrLandscapeToPlaceSplines->GetWorld(), Origin, BoxExtent, 4326, Coordinates))
		{
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("LoadGDALDatasetFromShortQuery2", "Internal error while reading coordinates. Make sure that your level coordinates are valid.")
			);
			if (OnComplete) OnComplete(nullptr);
			return;
		}
	}
	else
	{
		ALandscape *Landscape = Cast<ALandscape>(ActorOrLandscapeToPlaceSplines);

		FVector2D MinMaxX, MinMaxY, UnusedMinMaxZ;
		if (!LandscapeUtils::GetLandscapeBounds(Landscape, MinMaxX, MinMaxY, UnusedMinMaxZ))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("LoadGDALDatasetFromShortQuery3", "Could not compute bounds of Landscape {0}"),
				FText::FromString(Landscape->GetActorLabel())
			));
			if (OnComplete) OnComplete(nullptr);
			return;
		}

		FVector4d Locations;
		Locations[0] = MinMaxX[0];
		Locations[1] = MinMaxX[1];
		Locations[2] = MinMaxY[1];
		Locations[3] = MinMaxY[0];
		if (!ALevelCoordinates::GetEPSGCoordinatesFromUnrealLocations(Landscape->GetWorld(), Locations, 4326, Coordinates))
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				LOCTEXT("LoadGDALDatasetFromShortQuery3", "Could not compute coordinates of Landscape {0}"),
				FText::FromString(Landscape->GetActorLabel())
			));
			if (OnComplete) OnComplete(nullptr);
			return;
		}
	}

	// EPSG 4326
	double South = Coordinates[2];
	double West = Coordinates[0];
	double North = Coordinates[3];
	double East = Coordinates[1];
	LoadGDALDatasetFromQuery(Overpass::QueryFromShortQuery(South, West, North, East, ShortQuery), OnComplete);
}

void ASplineImporter::LoadGDALDataset(TFunction<void(GDALDataset*)> OnComplete)
{
	if (SplinesSource == ESourceKind::LocalFile)
	{
		if (OnComplete) OnComplete(LoadGDALDatasetFromFile(LocalFile));
	}
	else if (SplinesSource == ESourceKind::OverpassQuery)
	{	
		LoadGDALDatasetFromQuery(OverpassQuery, OnComplete);
	}
	else if (SplinesSource == ESourceKind::OverpassShortQuery)
	{
		LoadGDALDatasetFromShortQuery(OverpassShortQuery, OnComplete);
	}
	else if (SplinesSource == ESourceKind::Roads)
	{
		LoadGDALDatasetFromShortQuery(FString("way[\"highway\"][\"highway\"!~\"path\"][\"highway\"!~\"track\"];"), OnComplete);
	}
	else if (SplinesSource == ESourceKind::Buildings)
	{
		LoadGDALDatasetFromShortQuery(FString("way[\"building\"];"), OnComplete);
	}
	else if (SplinesSource == ESourceKind::Rivers)
	{
		LoadGDALDatasetFromShortQuery(FString("way[\"waterway\"=\"river\"];"), OnComplete);
	}
	else
	{
		check(false);
	}
}

void ASplineImporter::GenerateLandscapeSplines(
		ALandscape *Landscape,
		FCollisionQueryParams CollisionQueryParams,
		TArray<TArray<OGRPoint>> &PointLists
)
{
	FString LandscapeLabel = Landscape->GetActorLabel();
	ULandscapeSplinesComponent *LandscapeSplinesComponent = Landscape->GetSplinesComponent();
	if (!LandscapeSplinesComponent)
	{
		UE_LOG(LogSplineImporter, Log, TEXT("Did not find a landscape splines component. Creating one."));
		Landscape->CreateSplineComponent();
		LandscapeSplinesComponent = Landscape->GetSplinesComponent();
	}
	else
	{
		UE_LOG(LogSplineImporter, Log, TEXT("Found a landscape splines component"));
	}

	if (!LandscapeSplinesComponent)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("NoLandscapeSplinesComponent", "Could not create a landscape splines component for Landscape {0}."),
				FText::FromString(LandscapeLabel)
			)
		);
		return;
	}
		
	LandscapeSplinesComponent->Modify();
	LandscapeSplinesComponent->ShowSplineEditorMesh(true);

	TMap<FVector2D, ULandscapeSplineControlPoint*> Points;
	
	const int NumLists = PointLists.Num();
	FScopedSlowTask PointsTask = FScopedSlowTask(NumLists,
		FText::Format(
			LOCTEXT("PointsTask", "Adding points from {0} lines"),
			FText::AsNumber(NumLists)
		)
	);
	PointsTask.MakeDialog();

	for (auto &PointList : PointLists)
	{
		PointsTask.EnterProgressFrame();
		AddLandscapeSplinesPoints(Landscape, CollisionQueryParams, LandscapeSplinesComponent, PointList, Points);
	}

	UE_LOG(LogSplineImporter, Log, TEXT("Found %d control points"), Points.Num());
	
	FScopedSlowTask LandscapeSplinesTask = FScopedSlowTask(NumLists,
		FText::Format(
			LOCTEXT("PointsTask", "Adding landscape splines from {0} lines"),
			FText::AsNumber(NumLists)
		)
	);
	LandscapeSplinesTask.MakeDialog();
	
	for (auto &PointList : PointLists)
	{
		LandscapeSplinesTask.EnterProgressFrame();
		AddLandscapeSplines(Landscape, CollisionQueryParams, LandscapeSplinesComponent, PointList, Points);
	}
	
	UE_LOG(LogSplineImporter, Log, TEXT("Added %d segments"), LandscapeSplinesComponent->GetSegments().Num());
			
	LandscapeSplinesComponent->AttachToComponent(Landscape->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	LandscapeSplinesComponent->MarkRenderStateDirty();
	LandscapeSplinesComponent->PostEditChange();
}

void ASplineImporter::AddLandscapeSplinesPoints(
	ALandscape* Landscape,
	FCollisionQueryParams CollisionQueryParams,
	ULandscapeSplinesComponent* LandscapeSplinesComponent,
	TArray<OGRPoint> &PointList,
	TMap<FVector2D, ULandscapeSplineControlPoint*> &ControlPoints
)
{
	UWorld *World = Landscape->GetWorld();
	FTransform WorldToComponent = LandscapeSplinesComponent->GetComponentToWorld().Inverse();

	for (OGRPoint &Point : PointList)
	{
		double longitude = Point.getX();
		double latitude = Point.getY();

		FVector2D UnrealCoordinates;
		if (!ALevelCoordinates::GetUnrealCoordinatesFromEPSG(World, longitude, latitude, 4326, UnrealCoordinates))
		{	
			FMessageDialog::Open(EAppMsgType::Ok,
				LOCTEXT("NoLandscapeSplinesComponent", "Internal error while converting coordinates. Please check your LevelCoordinates actor")
			);
			return;
		}

		double x = UnrealCoordinates[0];
		double y = UnrealCoordinates[1];
		double z;
		if (LandscapeUtils::GetZ(World, CollisionQueryParams, x, y, z))
		{
			FVector Location = { x, y, z };
			FVector LocalLocation = WorldToComponent.TransformPosition(Location);
			ULandscapeSplineControlPoint* ControlPoint = NewObject<ULandscapeSplineControlPoint>(LandscapeSplinesComponent, NAME_None, RF_Transactional);
			ControlPoint->Location = LocalLocation;
			LandscapeSplinesComponent->GetControlPoints().Add(ControlPoint);
			ControlPoint->LayerName = "Road";
			ControlPoint->Width = 300; // half-width in cm
			ControlPoint->SideFalloff = 200;
			ControlPoints.Add( { longitude, latitude } , ControlPoint);
		}
	}
}

void ASplineImporter::AddLandscapeSplines(
	ALandscape* Landscape,
	FCollisionQueryParams CollisionQueryParams,
	ULandscapeSplinesComponent* LandscapeSplinesComponent,
	TArray<OGRPoint> &PointList,
	TMap<FVector2D, ULandscapeSplineControlPoint*> &Points
)
{
	int NumPoints = PointList.Num();
	for (int i = 0; i < NumPoints - 1; i++)
	{
		OGRPoint Point1 = PointList[i];
		OGRPoint Point2 = PointList[i + 1];

		FVector2D OGRLocation1 = { Point1.getX(), Point1.getY() };
		FVector2D OGRLocation2 = { Point2.getX(), Point2.getY() };

		// this may happen when GetZ returned false in `AddPoints`
		if (!Points.Contains(OGRLocation1) || !Points.Contains(OGRLocation2))
		{
			continue;
		}

		ULandscapeSplineControlPoint *ControlPoint1 = Points[OGRLocation1];
		ULandscapeSplineControlPoint *ControlPoint2 = Points[OGRLocation2];

		ControlPoint1->Modify();
		ControlPoint2->Modify();

		ULandscapeSplineSegment* NewSegment = NewObject<ULandscapeSplineSegment>(LandscapeSplinesComponent, NAME_None, RF_Transactional);
		LandscapeSplinesComponent->GetSegments().Add(NewSegment);
		
		NewSegment->LayerName = "Road";
		NewSegment->Connections[0].ControlPoint = ControlPoint1;
		NewSegment->Connections[1].ControlPoint = ControlPoint2;

		float TangentLen = (ControlPoint2->Location - ControlPoint1->Location).Size() / LandscapeSplinesStraightness;
		NewSegment->Connections[0].TangentLen = TangentLen;
		NewSegment->Connections[1].TangentLen = TangentLen;
		NewSegment->AutoFlipTangents();

		ControlPoint1->ConnectedSegments.Add(FLandscapeSplineConnection(NewSegment, 0));
		ControlPoint2->ConnectedSegments.Add(FLandscapeSplineConnection(NewSegment, 1));

		ControlPoint1->AutoCalcRotation();
		ControlPoint2->AutoCalcRotation();
		
		ControlPoint1->UpdateSplinePoints();
		ControlPoint2->UpdateSplinePoints();
	}
}

void ASplineImporter::GenerateRegularSplines(
	AActor *Actor,
	FCollisionQueryParams CollisionQueryParams,
	TArray<TArray<OGRPoint>> &PointLists
)
{
	UWorld *World = Actor->GetWorld();

	if (!World)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoWorld", "Internal error while creating splines: NULL World pointer.")
		);
		return;
	}

	ASplineCollection *SplineCollection = World->SpawnActor<ASplineCollection>();

	if (!SplineCollection)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoWorld", "Internal error while creating splines. Could not spawn a SplineCollection.")
		);
		return;
	}

	const int NumLists = PointLists.Num();
	FScopedSlowTask SplinesTask = FScopedSlowTask(NumLists,
		FText::Format(
			LOCTEXT("PointsTask", "Adding splines from {0} lines"),
			FText::AsNumber(NumLists)
		)
	);
	SplinesTask.MakeDialog();

	for (auto &PointList : PointLists)
	{
		SplinesTask.EnterProgressFrame();
		AddRegularSpline(Actor, SplineCollection, CollisionQueryParams, PointList);
	}
	
	GEditor->SelectActor(this, false, true);
	GEditor->SelectActor(SplineCollection, true, true, true, true);
	GEditor->NoteSelectionChange();
}

void ASplineImporter::AddRegularSpline(
	AActor* Actor,
	ASplineCollection* SplineCollection,
	FCollisionQueryParams CollisionQueryParams,
	TArray<OGRPoint> &PointList
)
{
	UWorld *World = Actor->GetWorld();
	
	int NumPoints = PointList.Num();
	if (NumPoints == 0) return;

	OGRPoint First = PointList[0];
	OGRPoint Last = PointList.Last();
			
	USplineComponent *SplineComponent = NewObject<USplineComponent>(SplineCollection);
	SplineCollection->SplineComponents.Add(SplineComponent);
	SplineComponent->RegisterComponent();
	SplineComponent->ClearSplinePoints();
	SplineComponent->SetMobility(EComponentMobility::Static);

	for (int i = 0; i < NumPoints; i++)
	{
		OGRPoint Point = PointList[i];
		double longitude = Point.getX();
		double latitude = Point.getY();

		if (Last != First || i < NumPoints - 1) // don't add last point in case the spline is a closed loop
		{
			FVector2D UnrealCoordinates;
			if (!ALevelCoordinates::GetUnrealCoordinatesFromEPSG(World, longitude, latitude, 4326, UnrealCoordinates))
			{	
				FMessageDialog::Open(EAppMsgType::Ok,
					LOCTEXT("NoLandscapeSplinesComponent", "Internal error while converting coordinates. Please check your LevelCoordinates actor")
				);
				return;
			}

			double x = UnrealCoordinates[0];
			double y = UnrealCoordinates[1];
			double z;
			if (LandscapeUtils::GetZ(World, CollisionQueryParams, x, y, z))
			{
				FVector Location = { x, y, z };
				SplineComponent->AddSplinePoint(Location, ESplineCoordinateSpace::World, false);
			}
		}
	}

	if (First == Last) SplineComponent->SetClosedLoop(true);
		

	if (SplinesSource == ESourceKind::Buildings)
	{
		auto &Points = SplineComponent->SplineCurves.Position.Points;
		for (FInterpCurvePoint<FVector> &Point : SplineComponent->SplineCurves.Position.Points)
		{
			Point.InterpMode = CIM_Linear;
		}
	}

	SplineComponent->UpdateSpline();
	SplineComponent->bSplineHasBeenEdited = true;

	SplineCollection->AddInstanceComponent(SplineComponent);
	SplineComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
}


#undef LOCTEXT_NAMESPACE