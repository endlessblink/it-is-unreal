#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles landscape/terrain manipulation commands:
 * - get_landscape_info: Get information about landscapes in the level
 * - sculpt_landscape: Raise or lower terrain at a location
 * - smooth_landscape: Smooth terrain at a location
 * - flatten_landscape: Flatten terrain at a location
 * - paint_landscape_layer: Paint a material layer on the terrain
 * - get_landscape_layers: Get available paint layers
 */
class UNREALMCP_API FEpicUnrealMCPLandscapeCommands
{
public:
    FEpicUnrealMCPLandscapeCommands();

    /** Main command dispatcher */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    /** Get information about all landscapes in the level */
    TSharedPtr<FJsonObject> HandleGetLandscapeInfo(const TSharedPtr<FJsonObject>& Params);

    /** Sculpt (raise/lower) terrain at a world location */
    TSharedPtr<FJsonObject> HandleSculptLandscape(const TSharedPtr<FJsonObject>& Params);

    /** Smooth terrain at a world location */
    TSharedPtr<FJsonObject> HandleSmoothLandscape(const TSharedPtr<FJsonObject>& Params);

    /** Flatten terrain at a world location to a specific height */
    TSharedPtr<FJsonObject> HandleFlattenLandscape(const TSharedPtr<FJsonObject>& Params);

    /** Paint a material layer on the terrain */
    TSharedPtr<FJsonObject> HandlePaintLandscapeLayer(const TSharedPtr<FJsonObject>& Params);

    /** Get available paint layers for a landscape */
    TSharedPtr<FJsonObject> HandleGetLandscapeLayers(const TSharedPtr<FJsonObject>& Params);

    /** Set landscape material */
    TSharedPtr<FJsonObject> HandleSetLandscapeMaterial(const TSharedPtr<FJsonObject>& Params);

    /** Create a landscape layer info object */
    TSharedPtr<FJsonObject> HandleCreateLandscapeLayer(const TSharedPtr<FJsonObject>& Params);

    /** Add a layer to a landscape */
    TSharedPtr<FJsonObject> HandleAddLayerToLandscape(const TSharedPtr<FJsonObject>& Params);

    /** Spawn a LandscapeParameterController actor for viewport-based MI control */
    TSharedPtr<FJsonObject> HandleSpawnLandscapeController(const TSharedPtr<FJsonObject>& Params);
};
