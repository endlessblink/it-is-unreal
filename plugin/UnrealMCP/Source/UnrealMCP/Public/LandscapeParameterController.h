#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceConstant.h"
#include "LandscapeParameterController.generated.h"

/**
 * Place this actor in the level to expose landscape material parameters
 * directly in the Details panel. Changing any slider here will immediately
 * update the linked Material Instance on the landscape.
 */
UCLASS(meta=(DisplayName="Landscape Parameter Controller"))
class UNREALMCP_API ALandscapeParameterController : public AActor
{
	GENERATED_BODY()

public:
	ALandscapeParameterController();

	/** Path to the Material Instance to control */
	UPROPERTY(EditAnywhere, Category="Target")
	FSoftObjectPath MaterialInstancePath;

	// --- Terrain Blending ---

	UPROPERTY(EditAnywhere, Category="Terrain Blending", meta=(ClampMin="0.1", ClampMax="20.0", ToolTip="How sharp the slope-based rock/ground transition is"))
	float SlopeSharpness = 3.0f;

	UPROPERTY(EditAnywhere, Category="Terrain Blending", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Amount of dry grass blended in"))
	float GrassAmount = 0.5f;

	UPROPERTY(EditAnywhere, Category="Terrain Blending", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Amount of rubble/gravel blended in"))
	float RubbleAmount = 0.3f;

	UPROPERTY(EditAnywhere, Category="Terrain Blending", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Amount of small pebbles blended in"))
	float PebbleAmount = 0.15f;

	// --- Surface Detail ---

	UPROPERTY(EditAnywhere, Category="Surface Detail", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Amount of dirt/soil patches (0=none)"))
	float DirtAmount = 0.0f;

	UPROPERTY(EditAnywhere, Category="Surface Detail", meta=(ClampMin="0.5", ClampMax="10.0", ToolTip="Dirt patch sharpness (higher=sharper edges)"))
	float DirtSize = 3.0f;

	UPROPERTY(EditAnywhere, Category="Surface Detail", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Amount of wet puddle patches (0=none)"))
	float PuddleAmount = 0.0f;

	UPROPERTY(EditAnywhere, Category="Surface Detail", meta=(ClampMin="1.0", ClampMax="20.0", ToolTip="Puddle edge sharpness (higher=sharper)"))
	float PuddleSize = 6.0f;

	// --- Material Properties ---

	UPROPERTY(EditAnywhere, Category="Material Properties", meta=(ClampMin="0.0", ClampMax="2.0", ToolTip="Normal map strength multiplier"))
	float NormalStrength = 1.0f;

	UPROPERTY(EditAnywhere, Category="Material Properties", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Surface roughness (0=mirror, 1=rough)"))
	float Roughness = 0.85f;

	UPROPERTY(EditAnywhere, Category="Material Properties", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Surface metallic amount"))
	float Metallic = 0.0f;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void SyncParametersToMI();
};
