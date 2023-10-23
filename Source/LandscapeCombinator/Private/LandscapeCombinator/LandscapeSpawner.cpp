// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "LandscapeCombinator/LandscapeSpawner.h"
#include "LandscapeCombinator/LogLandscapeCombinator.h"
#include "LandscapeCombinator/LandscapeController.h"
#include "LandscapeCombinator/Directories.h"

#include "LandscapeCombinator/HMLocalFile.h"
#include "LandscapeCombinator/HMLocalFolder.h"
#include "LandscapeCombinator/HMURL.h"

#include "LandscapeCombinator/HMRGEALTI.h"
#include "LandscapeCombinator/HMSwissALTI3DRenamer.h"
#include "LandscapeCombinator/HMLitto3DGuadeloupe.h"
#include "LandscapeCombinator/HMLitto3DGuadeloupeRenamer.h"
#include "LandscapeCombinator/HMDegreeRenamer.h"
#include "LandscapeCombinator/HMViewfinder15Downloader.h"
#include "LandscapeCombinator/HMViewfinder15Renamer.h"
#include "LandscapeCombinator/HMViewfinderDownloader.h"

#include "LandscapeCombinator/HMDebugFetcher.h"
#include "LandscapeCombinator/HMPreprocess.h"
#include "LandscapeCombinator/HMResolution.h"
#include "LandscapeCombinator/HMReproject.h"
#include "LandscapeCombinator/HMToPNG.h"
#include "LandscapeCombinator/HMSetEPSG.h"
#include "LandscapeCombinator/HMConvert.h"
#include "LandscapeCombinator/HMAddMissingTiles.h"
#include "LandscapeCombinator/HMListDownloader.h"

#include "HeightmapModifier/HeightmapModifier.h"
#include "LandscapeUtils/LandscapeUtils.h"
#include "Coordinates/LevelCoordinates.h"
#include "GDALInterface/GDALInterface.h"
#include "HAL/FileManagerGeneric.h"

#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

ALandscapeSpawner::ALandscapeSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	PreprocessingTool = CreateDefaultSubobject<UExternalTool>(TEXT("Preprocessing Tool"));
}

HMFetcher* ALandscapeSpawner::CreateInitialFetcher()
{
	switch (HeightMapSourceKind)
	{
		case EHeightMapSourceKind::LocalFile:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("LocalFile", new HMLocalFile(LocalFilePath), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("SetEPSG", new HMSetEPSG());
			return Fetcher1->AndThen(Fetcher2);
		}

		case EHeightMapSourceKind::LocalFolder:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("LocalFolder", new HMLocalFolder(Folder), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("SetEPSG", new HMSetEPSG());
			return Fetcher1->AndThen(Fetcher2);
		}

		case EHeightMapSourceKind::URL:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("URL", new HMURL(URL, LandscapeLabel), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("SetEPSG", new HMSetEPSG());
			return Fetcher1->AndThen(Fetcher2);
		}

		case EHeightMapSourceKind::Litto3D_Guadeloupe:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("Litto3DGuadeloupe", new HMLitto3DGuadeloupe(Litto3D_Folder, bUse5mData, bSkipExtraction), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("Litto3DGuadeloupeRenamer", new HMLitto3DGuadeloupeRenamer(LandscapeLabel));
			return Fetcher1->AndThen(Fetcher2);
		}

		case EHeightMapSourceKind::RGEALTI:
		{
			return new HMDebugFetcher("RGEALTI", HMRGEALTI::RGEALTI(LandscapeLabel, RGEALTIMinLong, RGEALTIMaxLong, RGEALTIMinLat, RGEALTIMaxLat, bResizeRGEAltiUsingWebAPI, RGEALTIWidth, RGEALTIHeight), true);
		}

		case EHeightMapSourceKind::Viewfinder15:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("ViewfinderDownloader", new HMViewfinderDownloader(Viewfinder15_TilesString, "http://www.viewfinderpanoramas.org/DEM/TIF15/", true), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("Viewfinder15Renamer", new HMViewfinder15Renamer(LandscapeLabel));
			return Fetcher1->AndThen(Fetcher2);
		}

		case EHeightMapSourceKind::Viewfinder3:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("ViewfinderDownloader", new HMViewfinderDownloader(Viewfinder3_TilesString, "http://viewfinderpanoramas.org/dem3/", false), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("Convert", new HMConvert(LandscapeLabel, "tif"));
			HMFetcher *Fetcher3 = new HMDebugFetcher("Viewfinder1or3Renamer", new HMDegreeRenamer(LandscapeLabel));
			return Fetcher1->AndThen(Fetcher2)->AndThen(Fetcher3);
		}

		case EHeightMapSourceKind::Viewfinder1:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("ViewfinderDownloader", new HMViewfinderDownloader(Viewfinder1_TilesString, "http://viewfinderpanoramas.org/dem1/", false), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("Convert", new HMConvert(LandscapeLabel, "tif"));
			HMFetcher *Fetcher3 = new HMDebugFetcher("Viewfinder1or3Renamer", new HMDegreeRenamer(LandscapeLabel));
			return Fetcher1->AndThen(Fetcher2)->AndThen(Fetcher3);
		}

		case EHeightMapSourceKind::SwissALTI_3D:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("ListDownloader", new HMListDownloader(SwissALTI3DListOfLinks), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("SetEPSG", new HMSetEPSG());
			HMFetcher *Fetcher3 = new HMDebugFetcher("SwissALTI3DRenamer", new HMSwissALTI3DRenamer(LandscapeLabel));
			return Fetcher1->AndThen(Fetcher2)->AndThen(Fetcher3);
		}

		case EHeightMapSourceKind::USGS_OneThird:
		{
			HMFetcher *Fetcher1 = new HMDebugFetcher("ListDownloader", new HMListDownloader(USGS_OneThirdListOfLinks), true);
			HMFetcher *Fetcher2 = new HMDebugFetcher("SetEPSG", new HMSetEPSG());
			HMFetcher *Fetcher3 = new HMDebugFetcher("DegreeRenamer", new HMDegreeRenamer(LandscapeLabel));
			return Fetcher1->AndThen(Fetcher2)->AndThen(Fetcher3);
		}

		default:
			FMessageDialog::Open(EAppMsgType::Ok,
				FText::Format(
					LOCTEXT("InterfaceFromKindError", "Internal error: heightmap kind '{0}' is not supprted."),
					FText::AsNumber((int) HeightMapSourceKind)
				)
			); 
			return nullptr;
	}
}

HMFetcher* ALandscapeSpawner::CreateFetcher(HMFetcher *InitialFetcher)
{
	if (!InitialFetcher) return nullptr;

	HMFetcher *Result = InitialFetcher;
	
	if (bPreprocess)
	{
		Result = Result->AndThen(new HMDebugFetcher("Preprocess", new HMPreprocess(LandscapeLabel, PreprocessingTool)));
	}

	if (!bSetLevelCoordinatesWorldOrigin && bRequiresReprojection)
	{
		TObjectPtr<UGlobalCoordinates> GlobalCoordinates = ALevelCoordinates::GetGlobalCoordinates(this->GetWorld());

		if (!GlobalCoordinates) return nullptr;

		Result = Result->AndThen(new HMDebugFetcher("Reproject", new HMReproject(LandscapeLabel, GlobalCoordinates->EPSG)));
	}

	Result = Result->AndRun([this](HMFetcher *FetcherBeforePNG)
		{
			GDALInterface::GetMinMax(this->Altitudes, FetcherBeforePNG->OutputFiles);
		}
	);

	if (bConvertToPNG)
	{
		Result = Result->AndThen(new HMDebugFetcher("ToPNG", new HMToPNG(LandscapeLabel)));

		// we apply the Viewfinder renamer after converting to PNG
		//if (!bRequiresReprojection && (HeightMapSourceKind == EHeightMapSourceKind::Viewfinder1 || HeightMapSourceKind == EHeightMapSourceKind::Viewfinder3))
		//{
		//	Result = Result->AndThen(new HMDebugFetcher("Viewfinder1or3Renamer", new HMDegreeRenamer(LandscapeLabel)));
		//}

		Result = Result->AndThen(new HMDebugFetcher("AddMissingTiles", new HMAddMissingTiles()));
	}

	if (bChangeResolution)
	{
		Result = Result->AndThen(new HMDebugFetcher("Resolution", new HMResolution(LandscapeLabel, PrecisionPercent)));
	}

	return Result;
}

bool GetPixels(FIntPoint& InsidePixels, TArray<FString> Files)
{
	if (Files.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ALandscapeSpawner::GetPixels", "Internal Landscape Combinator error, empty list of files when trying to read the size")
		); 
		return false;
	}

	if (!GDALInterface::GetPixels(InsidePixels, Files[0])) return false;

	if (Files.Num() == 1) return true;

	HMTilesCounter TilesCounter(Files);
	TilesCounter.ComputeMinMaxTiles();
	InsidePixels[0] *= (TilesCounter.LastTileX + 1);
	InsidePixels[1] *= (TilesCounter.LastTileY + 1);
	return true;
}


void ALandscapeSpawner::SpawnLandscape()
{
	HMFetcher *InitialFetcher = CreateInitialFetcher();
	HMFetcher* Fetcher = CreateFetcher(InitialFetcher);

	if (!Fetcher)
	{			
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("ErrorBound", "There was an error while creating heightmap files for Landscape {0}."),
				FText::FromString(LandscapeLabel)
			)
		); 
		return;
	}

	Fetcher->Fetch(0, TArray<FString>(), [InitialFetcher, Fetcher, this](bool bSuccess)
	{
		// GameThread to spawn a landscape
		AsyncTask(ENamedThreads::GameThread, [bSuccess, InitialFetcher, Fetcher, this]()
		{	
			if (bSuccess)
			{
				ALandscape *CreatedLandscape = LandscapeUtils::SpawnLandscape(Fetcher->OutputFiles, LandscapeLabel, bDropData);

				if (CreatedLandscape)
				{
					CreatedLandscape->SetActorLabel(LandscapeLabel);

					FVector4d OriginalCoordinates;
					if (!GDALInterface::GetCoordinates(OriginalCoordinates, InitialFetcher->OutputFiles))
					{
						FMessageDialog::Open(EAppMsgType::Ok,
							FText::Format(
								LOCTEXT("ErrorBound", "There was an internal error while getting coordinates for Landscape {0}."),
								FText::FromString(LandscapeLabel)
							)
						);
						// delete this; // FIXME: destroy Fetcher
						return;
					}
					
					TObjectPtr<UGlobalCoordinates> GlobalCoordinates = ALevelCoordinates::GetGlobalCoordinates(this->GetWorld());
					if (bSetLevelCoordinatesWorldOrigin)
					{
						FVector4d Coordinates;
						if (
							!GlobalCoordinates ||
							!GDALInterface::ConvertCoordinates(OriginalCoordinates, Coordinates, InitialFetcher->OutputEPSG, GlobalCoordinates->EPSG)
						)
						{
							FMessageDialog::Open(EAppMsgType::Ok,
								FText::Format(
									LOCTEXT("ErrorBound", "There was an internal error while setting Global Coordinates world origin from Landscape {0}."),
									FText::FromString(LandscapeLabel)
								)
							);
							// delete this; // FIXME: destroy Fetcher
							return;
						}

						double MinCoordWidth = Coordinates[0];
						double MaxCoordWidth = Coordinates[1];
						double MinCoordHeight = Coordinates[2];
						double MaxCoordHeight = Coordinates[3];
						GlobalCoordinates->WorldOriginLong = (MinCoordWidth + MaxCoordWidth) / 2;
						GlobalCoordinates->WorldOriginLat = (MinCoordHeight + MaxCoordHeight) / 2;
						GlobalCoordinates->EPSG = Fetcher->OutputEPSG;
					}

					ULandscapeController *LandscapeController = NewObject<ULandscapeController>(CreatedLandscape->GetRootComponent());
					LandscapeController->RegisterComponent();
					CreatedLandscape->AddInstanceComponent(LandscapeController);

					UHeightmapModifier *HeightmapModifier = NewObject<UHeightmapModifier>(CreatedLandscape->GetRootComponent());
					HeightmapModifier->RegisterComponent();
					CreatedLandscape->AddInstanceComponent(HeightmapModifier);

					LandscapeController->OriginalCoordinates = OriginalCoordinates;
					LandscapeController->OriginalEPSG = InitialFetcher->OutputEPSG;
					LandscapeController->Altitudes = Altitudes;
					GetPixels(LandscapeController->InsidePixels, Fetcher->OutputFiles);
					LandscapeController->ZScale = ZScale;

					LandscapeController->AdjustLandscape();

					UE_LOG(LogLandscapeCombinator, Log, TEXT("Created Landscape %s successfully."), *LandscapeLabel);
					FMessageDialog::Open(EAppMsgType::Ok,
						FText::Format(
							LOCTEXT("LandscapeCreated", "Landscape {0} was created successfully"),
							FText::FromString(LandscapeLabel)
						)
					);
				}
				else
				{
					UE_LOG(LogLandscapeCombinator, Error, TEXT("Could not create Landscape %s."), *LandscapeLabel);
					FMessageDialog::Open(EAppMsgType::Ok,
						FText::Format(
							LOCTEXT("LandscapeNotCreated", "Landscape {0} could not be created."),
							FText::FromString(LandscapeLabel)
						)
					);
				}
			}
			else
			{
				UE_LOG(LogLandscapeCombinator, Error, TEXT("Could not create heightmaps files for Landscape %s."), *LandscapeLabel);
				FMessageDialog::Open(EAppMsgType::Ok,
					FText::Format(
						LOCTEXT("LandscapeCreated", "Could not create heightmaps files for Landscape {0}."),
						FText::FromString(LandscapeLabel)
					)
				);
			}
			
			//delete this; // FIXME: destroy Fetcher
		});
	});

	return;
}

void ALandscapeSpawner::DeleteAllHeightmaps()
{
	FString LandscapeCombinatorDir = Directories::LandscapeCombinatorDir();
	if (!LandscapeCombinatorDir.IsEmpty())
	{
		IPlatformFile::GetPlatformPhysical().DeleteDirectoryRecursively(*LandscapeCombinatorDir);
	}

	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Deleting", "Finished Deleting files."));
}

void ALandscapeSpawner::DeleteAllProcessedHeightmaps()
{
	TArray<FString> FilesAndFolders;

	FString LandscapeCombinatorDir = Directories::LandscapeCombinatorDir();

	UE_LOG(LogLandscapeCombinator, Log, TEXT("DeleteAllProcessedHeightmaps"));

	if (!LandscapeCombinatorDir.IsEmpty())
	{
		FFileManagerGeneric::Get().FindFiles(FilesAndFolders, *FPaths::Combine(LandscapeCombinatorDir, FString("*")), true, true);
		UE_LOG(LogLandscapeCombinator, Log, TEXT("Deleting %d files and folders in %s"), FilesAndFolders.Num(), *LandscapeCombinatorDir);
		for (auto& File0 : FilesAndFolders)
		{
			if (!File0.Equals("Download"))
			{
				FString File = FPaths::Combine(LandscapeCombinatorDir, File0);
				UE_LOG(LogLandscapeCombinator, Log, TEXT("Deleting %s."), *File);
				IPlatformFile::GetPlatformPhysical().DeleteFile(*File);
				IPlatformFile::GetPlatformPhysical().DeleteDirectoryRecursively(*File);
			}
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Deleting", "Finished Deleting files."));
}

#undef LOCTEXT_NAMESPACE