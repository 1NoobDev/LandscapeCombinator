// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "HeightmapModifier/HeightmapModifier.h"
#include "HeightmapModifier/LogHeightmapModifier.h"

#include "LandscapeUtils/LandscapeUtils.h"
#include "GDALInterface/GDALInterface.h"

#include "Kismet/GameplayStatics.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeDataAccess.h"



#define LOCTEXT_NAMESPACE "FHeightmapModifierModule"

UHeightmapModifier::UHeightmapModifier()
{
	ExternalTool = CreateDefaultSubobject<UExternalTool>(TEXT("External Tool"));
}

void UHeightmapModifier::BlendWithOtherLandscapes()
{
	ALandscape *Landscape = Cast<ALandscape>(GetOwner());
	if (!Landscape) return;

	TArray<AActor*> OtherLandscapes;
	UGameplayStatics::GetAllActorsOfClass(this->GetWorld(), ALandscape::StaticClass(), OtherLandscapes);

	FVector2D MinMaxX, MinMaxY, UnusedMinMaxZ;
	if (!LandscapeUtils::GetLandscapeBounds(Landscape, MinMaxX, MinMaxY, UnusedMinMaxZ))
	{
		return;
	}

	double ExtentX = MinMaxX[1] - MinMaxX[0];
	double ExtentY = MinMaxY[1] - MinMaxY[0];
	
	FHeightmapAccessor<false> HeightmapAccessor(Landscape->GetLandscapeInfo());

	int32 SizeX = Landscape->ComputeComponentCounts().X * Landscape->ComponentSizeQuads + 1;
	int32 SizeY = Landscape->ComputeComponentCounts().Y * Landscape->ComponentSizeQuads + 1;
	uint16* HeightmapData = (uint16*) malloc(SizeX * SizeY * (sizeof uint16));
	HeightmapAccessor.GetDataFast(0, 0, SizeX - 1, SizeY - 1, HeightmapData);


	FVector GlobalTopLeft = FVector(MinMaxX[0], MinMaxY[0], 0);
	FVector GlobalBottomRight = FVector(MinMaxX[1], MinMaxY[1], 0);

	FTransform ThisToGlobal = Landscape->GetTransform();
	FTransform GlobalToThis = ThisToGlobal.Inverse();

	const FScopedTransaction Transaction(LOCTEXT("BlendWithOtherLandscapes", "Blending With Other Landscapes"));

	for (auto& OtherLandscape0 : OtherLandscapes)
	{
		ALandscape *OtherLandscape = Cast<ALandscape>(OtherLandscape0);
		if (OtherLandscape == Landscape) continue;

		FVector2D OtherMinMaxX, OtherMinMaxY, UnusedOtherMinMaxZ;
		if (!LandscapeUtils::GetLandscapeBounds(OtherLandscape, OtherMinMaxX, OtherMinMaxY, UnusedOtherMinMaxZ))
		{
			return;
		}

		if (
			MinMaxX[0] <= OtherMinMaxX[1] && OtherMinMaxX[0] <= MinMaxX[1] &&
			MinMaxY[0] <= OtherMinMaxY[1] && OtherMinMaxY[0] <= MinMaxY[1]
		)
		{
			FTransform OtherToGlobal = OtherLandscape->GetTransform();
			FTransform GlobalToOther = OtherToGlobal.Inverse();
			FVector OtherTopLeft = GlobalToOther.TransformPosition(GlobalTopLeft);
			FVector OtherBottomRight = GlobalToOther.TransformPosition(GlobalBottomRight);
			
			FHeightmapAccessor<false> OtherHeightmapAccessor(OtherLandscape->GetLandscapeInfo());

			// We are only interested in the heightmap data from `OtherLandscape` in the rectangle delimited by
			// the `TopLeft` and `BottomRight` corners 
			
			int32 OtherTotalSizeX = OtherLandscape->ComputeComponentCounts().X * OtherLandscape->ComponentSizeQuads + 1;
			int32 OtherTotalSizeY = OtherLandscape->ComputeComponentCounts().Y * OtherLandscape->ComponentSizeQuads + 1;
			int32 OtherX1 = FMath::Min(OtherTotalSizeX - 1, FMath::Max(0, OtherTopLeft.X + 1));
			int32 OtherX2 = FMath::Min(OtherTotalSizeX - 1, FMath::Max(0, OtherBottomRight.X - 1));
			int32 OtherY1 = FMath::Min(OtherTotalSizeY - 1, FMath::Max(0, OtherTopLeft.Y + 1));
			int32 OtherY2 = FMath::Min(OtherTotalSizeY - 1, FMath::Max(0, OtherBottomRight.Y - 1));
			int32 OtherSizeX = OtherX2 - OtherX1 + 1;
			int32 OtherSizeY = OtherY2 - OtherY1 + 1;

			if (OtherSizeX <= 0 || OtherSizeY <= 0)
			{
				UE_LOG(LogHeightmapModifier, Log, TEXT("Could not blend with Landscape %s"), *OtherLandscape->GetActorLabel());
				continue;
			}

			UE_LOG(LogHeightmapModifier, Log, TEXT("Blending with Landscape %s (MinX: %d, MaxX: %d, MinY: %d, MaxY: %d)"),
				*OtherLandscape->GetActorLabel(), OtherX1, OtherX2, OtherY1, OtherY2
			);

			uint16* OtherOldHeightmapData = (uint16*) malloc(OtherSizeX * OtherSizeY * (sizeof uint16));
			uint16* OtherNewHeightmapData = (uint16*) malloc(OtherSizeX * OtherSizeY * (sizeof uint16));
			OtherHeightmapAccessor.GetDataFast(OtherX1, OtherY1, OtherX2, OtherY2, OtherOldHeightmapData);

			for (int X = 0; X < OtherSizeX; X++)
			{
				for (int Y = 0; Y < OtherSizeY; Y++)
				{
					FVector OtherPosition = FVector(OtherX1 + X, OtherY1 + Y, 0);
					FVector GlobalPosition = OtherToGlobal.TransformPosition(OtherPosition);
					FVector ThisPosition = GlobalToThis.TransformPosition(GlobalPosition);

					int ThisX = ThisPosition.X;
					int ThisY = ThisPosition.Y;

					// if this landscape has non-zero data at this position
					if (ThisX >= 0 && ThisY >= 0 && ThisX < SizeX && ThisY < SizeY && HeightmapData[ThisX + ThisY * SizeX] > 0)
					{
						// we delete the data from the other landscape
						OtherNewHeightmapData[X + Y * OtherSizeX] = 0;
					}
					else
					{
						// otherwise, we keep the old data
						OtherNewHeightmapData[X + Y * OtherSizeX] = OtherOldHeightmapData[X + Y * OtherSizeX];
					}
				}
			}

			OtherHeightmapAccessor.SetData(OtherX1, OtherY1, OtherX2, OtherY2, OtherNewHeightmapData);

			delete OtherNewHeightmapData;
			delete OtherOldHeightmapData;

			UE_LOG(LogHeightmapModifier, Log, TEXT("Finished blending with Landscape %s (MinX: %d, MaxX: %d, MinY: %d, MaxY: %d)"),
				*OtherLandscape->GetActorLabel(), OtherX1, OtherX2, OtherY1, OtherY2
			);
		}
		else
		{
			UE_LOG(LogHeightmapModifier, Log, TEXT("Skipping blending with Landscape %s, which does not overlap with Landscape %s"),
				*OtherLandscape->GetActorLabel(),
				*Landscape->GetActorLabel()
			);
		}

	}

	delete HeightmapData;
	return;
}

void UHeightmapModifier::ApplyToolToHeightmap()
{
	ALandscape *Landscape = Cast<ALandscape>(GetOwner());
	if (!Landscape)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::1", "The owner of HeightmapModifier must be a landscape.")
		); 
		return;
	}

	FString LandscapeLabel = Landscape->GetActorLabel();

	if (Landscape->IsMaxLayersReached())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::Layers", "Landscape {0} has too many edit layers, please delete one.")
		); 
		return;
	}

	if (!BoundingActor)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::2", "Please select a bounding actor.")
		); 
		return;
	}

	
	/* Get the heightmap data from `Landscape` using `BoundingActor` as bounds */

	FTransform GlobalToThis = Landscape->GetTransform().Inverse();

	FVector Origin, Extent;
	BoundingActor->GetActorBounds(false, Origin, Extent);

	int Top = Origin.Y - Extent.Y;
	int Bottom = Origin.Y + Extent.Y;
	int Left = Origin.X - Extent.X;
	int Right = Origin.X + Extent.X;

	FVector TopLeft(Left, Top, 0);
	FVector BottomRight(Right, Bottom, 0);
	
	FVector LocalTopLeft = GlobalToThis.TransformPosition(TopLeft);
	FVector LocalBottomRight = GlobalToThis.TransformPosition(BottomRight);
	
	int32 TotalSizeX = Landscape->ComputeComponentCounts().X * Landscape->ComponentSizeQuads + 1;
	int32 TotalSizeY = Landscape->ComputeComponentCounts().Y * Landscape->ComponentSizeQuads + 1;

	int32 X1 = FMath::Min(TotalSizeX - 1, FMath::Max(0, LocalTopLeft.X));
	int32 X2 = FMath::Min(TotalSizeX - 1, FMath::Max(0, LocalBottomRight.X));
	int32 Y1 = FMath::Min(TotalSizeY - 1, FMath::Max(0, LocalTopLeft.Y));
	int32 Y2 = FMath::Min(TotalSizeY - 1, FMath::Max(0, LocalBottomRight.Y));
	int32 SizeX = X2 - X1 + 1;
	int32 SizeY = Y2 - Y1 + 1;

	if (SizeX > INT32_MAX / SizeY)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::28", "The size of the area to edit is too large.")
		);
		return;
	}
	
	uint16* HeightmapData = (uint16*) malloc(SizeX * SizeY * (sizeof uint16));
	if (!HeightmapData)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::29", "Not enough memory to allocate for new heightmap data.")
		);
		return;
	}
	
	ULandscapeInfo *LandscapeInfo = Landscape->GetLandscapeInfo();
	if (!LandscapeInfo)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::3", "Could not get LandscapeInfo for Landscape {0}."),
			FText::FromString(LandscapeLabel)
		)); 

		delete HeightmapData;
		return;
	}

	FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
	HeightmapAccessor.GetDataFast(X1, Y1, X2, Y2, HeightmapData);


	/* Prepare the directories */
	
	FString Intermediate = FPaths::ConvertRelativePathToFull(FPaths::EngineIntermediateDir());
	FString TempDir = FPaths::Combine(Intermediate, "Temp");
	if (!IPlatformFile::GetPlatformPhysical().CreateDirectory(*TempDir))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::4", "Could not initialize directory {0}."),
			FText::FromString(TempDir)
		));

		delete HeightmapData;
		return;
	}


	/* Prepare the filenames */

	FString InputFile = FPaths::Combine(TempDir, "input.tif");
	FString Extension = ExternalTool->bChangeExtension ? ExternalTool->NewExtension : "tif";
	FString OutputFile = FPaths::Combine(TempDir, "output." + Extension);


	/* Prepare GDAL Drivers */

	GDALDriver *MEMDriver = GetGDALDriverManager()->GetDriverByName("MEM");
	GDALDriver *TIFDriver = GetGDALDriverManager()->GetDriverByName("GTiff");

	if (!TIFDriver || !MEMDriver)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::3", "Could not load GDAL drivers.")
		);
		delete HeightmapData;
		return;
	}


	/* Write the heightmap data to `InputFile` */

	GDALDataset *Dataset = MEMDriver->Create(
		"",
		SizeX, SizeY,
		1, // nBands
		GDT_UInt16,
		nullptr
	);

	if (!Dataset)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::4", "There was an error while creating a GDAL Dataset."),
			FText::FromString(InputFile)
		));
		delete HeightmapData;
		return;
	}

	if (Dataset->GetRasterCount() != 1)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::4", "There was an error while writing heightmap data to file {0}."),
			FText::FromString(InputFile)
		));
		delete HeightmapData;
		GDALClose(Dataset);
		return;
	}


	CPLErr WriteErr = Dataset->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, SizeX, SizeY, HeightmapData, SizeX, SizeY, GDT_UInt16, 0, 0);
	delete HeightmapData;

	if (WriteErr != CE_None)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::5", "There was an error while writing heightmap data to file {0}. (Error: {1})"),
			FText::FromString(InputFile),
			FText::AsNumber(WriteErr)
		));
		GDALClose(Dataset);
		return;
	}

	GDALDataset *TIFDataset = TIFDriver->CreateCopy(TCHAR_TO_UTF8(*InputFile), Dataset, 1, nullptr, nullptr, nullptr);
	GDALClose(Dataset);

	if (!TIFDataset)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::5", "Could not write heightmap to file {0}."),
			FText::FromString(InputFile),
			FText::AsNumber(WriteErr)
		));
		return;
	}
	
	GDALClose(TIFDataset);


	/* Run the External Tool from `InputFile` to `OutputFile` */

	if (!ExternalTool->Run(InputFile, OutputFile))
	{
		return;
	}

	/* Read the new data from `OutputFile` */

	GDALDataset *NewDataset = (GDALDataset *)GDALOpen(TCHAR_TO_UTF8(*OutputFile), GA_ReadOnly);

	if (!NewDataset)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::6", "Could not read file {0} using GDAL."),
			FText::FromString(OutputFile)
		));
		return;
	}
	
	uint16* NewHeightmapData = (uint16*) malloc(SizeX * SizeY * (sizeof uint16));

	if (!NewHeightmapData)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::7", "Not enough memory to allocate for new heightmap data."),
			FText::FromString(OutputFile)
		));
		GDALClose(NewDataset);
		return;
	}

	CPLErr ReadErr = NewDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, SizeX, SizeY, NewHeightmapData, SizeX, SizeY, GDT_UInt16, 0, 0);
	GDALClose(NewDataset);

	if (ReadErr != CE_None)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::7", "There was an error while reading heightmap data from file {0}."),
			FText::FromString(OutputFile)
		));
		delete NewHeightmapData;
		return;
	}


	/* Make the new data to be a difference, so that it can be used on a different edit layer */
	
	for (int i = 0; i < SizeX; i++)
	{
		for (int j = 0; j < SizeY; j++)
		{
			NewHeightmapData[i + j * SizeX] -= HeightmapData[i + j * SizeX];
		}
	}


	/* Write difference data to a new edit layer. */

	int LayerIndex = Landscape->CreateLayer();
	if (LayerIndex == INDEX_NONE)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
			LOCTEXT("UHeightmapModifier::ModifyHeightmap::9", "Could not create landscape layer. Make sure that edit layers are enabled on Landscape {0}."),
			FText::FromString(LandscapeLabel)
		));
		delete NewHeightmapData;
		return;
	}

	HeightmapAccessor.SetEditLayer(Landscape->GetLayer(LayerIndex)->Guid);
	HeightmapAccessor.SetData(X1, Y1, X2, Y2, NewHeightmapData);
	delete NewHeightmapData;
	
	FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
		LOCTEXT("UHeightmapModifier::ModifyHeightmap::10", "Finished applying command {0} on the Landscape {1}."),
		FText::FromString(ExternalTool->Command),
		FText::FromString(LandscapeLabel)
	));
}

#undef LOCTEXT_NAMESPACE