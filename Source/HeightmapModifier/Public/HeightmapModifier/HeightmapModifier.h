// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#pragma once

#include "ConsoleHelpers/ExternalTool.h"
#include "CoreMinimal.h"

#include "HeightmapModifier.generated.h"

UCLASS(BlueprintType)
class HEIGHTMAPMODIFIER_API UHeightmapModifier : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeightmapModifier();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "HeightmapModifier|ExternalTool")
	void ApplyToolToHeightmap();

	UPROPERTY(EditAnywhere, Category = "HeightmapModifier|ExternalTool",
		meta = (DisplayPriority = "1")
	)
	/* Use a cube or another rectangular actor to bound the area on your landscape on which you want to apply the modification */
	AActor* BoundingActor;
	
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "HeightmapModifier|ExternalTool",
		meta = (DisplayPriority = "11")
	)
	TObjectPtr<UExternalTool> ExternalTool;
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "HeightmapModifier|Blend")
	void BlendWithOtherLandscapes();
};
