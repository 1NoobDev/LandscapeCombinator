// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#pragma once

#include "DecalCoordinates.generated.h"

#define LOCTEXT_NAMESPACE "FCoordinatesModule"

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class COORDINATES_API UDecalCoordinates : public UActorComponent
{
	GENERATED_BODY()

public:

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "DecalCoordinates",
		meta = (DisplayPriority = "1")
	)
	FString PathToGeoreferencedImage;

	/* Move the Decal Actor to the correct position with respect to the global coordinate system. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "DecalCoordinates",
		meta = (DisplayPriority = "5")
	)
	void PlaceDecal();
};

#undef LOCTEXT_NAMESPACE