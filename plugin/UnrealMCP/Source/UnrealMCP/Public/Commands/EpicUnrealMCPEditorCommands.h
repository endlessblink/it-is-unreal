#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Editor-related MCP commands
 * Handles viewport control, actor manipulation, and level management
 */
class UNREALMCP_API FEpicUnrealMCPEditorCommands
{
public:
    	FEpicUnrealMCPEditorCommands();

    // Handle editor commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Actor manipulation commands
    TSharedPtr<FJsonObject> HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDeleteActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params);

    // Blueprint actor spawning
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);

    // Actor property manipulation
    TSharedPtr<FJsonObject> HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params);

    // Material commands
    TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetMaterialInstanceParameter(const TSharedPtr<FJsonObject>& Params);

    // Texture commands
    TSharedPtr<FJsonObject> HandleImportTexture(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetTextureProperties(const TSharedPtr<FJsonObject>& Params);

    // PBR Material creation
    TSharedPtr<FJsonObject> HandleCreatePBRMaterial(const TSharedPtr<FJsonObject>& Params);

    // Landscape material creation (atomic, builds entire graph in one call)
    TSharedPtr<FJsonObject> HandleCreateLandscapeMaterial(const TSharedPtr<FJsonObject>& Params);

    // Asset import and management commands
    TSharedPtr<FJsonObject> HandleImportMesh(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleImportSkeletalMesh(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleImportAnimation(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleListAssets(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleDoesAssetExist(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetAssetInfo(const TSharedPtr<FJsonObject>& Params);

    // World query commands
    TSharedPtr<FJsonObject> HandleGetHeightAtLocation(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSnapActorToGround(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleScatterMeshesOnLandscape(const TSharedPtr<FJsonObject>& Params);

    // Screenshot capture
    TSharedPtr<FJsonObject> HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params);

    // Debugging / inspection tools
    TSharedPtr<FJsonObject> HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleFocusViewportOnActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetTextureInfo(const TSharedPtr<FJsonObject>& Params);

    // Bulk operations
    TSharedPtr<FJsonObject> HandleDeleteActorsByPattern(const TSharedPtr<FJsonObject>& Params);

    // Asset deletion
    TSharedPtr<FJsonObject> HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params);

    // Mesh asset properties
    TSharedPtr<FJsonObject> HandleSetNaniteEnabled(const TSharedPtr<FJsonObject>& Params);

    // HISM-based foliage scatter (Poisson disk + slope filter)
    TSharedPtr<FJsonObject> HandleScatterFoliage(const TSharedPtr<FJsonObject>& Params);

    // Audio import
    TSharedPtr<FJsonObject> HandleImportSound(const TSharedPtr<FJsonObject>& Params);

    // Animation notify
    TSharedPtr<FJsonObject> HandleAddAnimNotify(const TSharedPtr<FJsonObject>& Params);

    // Editor log reading
    TSharedPtr<FJsonObject> HandleGetEditorLog(const TSharedPtr<FJsonObject>& Params);
};