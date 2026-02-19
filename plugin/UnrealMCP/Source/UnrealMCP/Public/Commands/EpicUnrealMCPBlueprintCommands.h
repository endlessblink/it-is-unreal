#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Blueprint-related MCP commands
 */
class FEpicUnrealMCPBlueprintCommands
{
public:
    	FEpicUnrealMCPBlueprintCommands();

    // Handle blueprint commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Specific blueprint command handlers (only used functions)
    TSharedPtr<FJsonObject> HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetMeshMaterialColor(const TSharedPtr<FJsonObject>& Params);
    
    // Material management functions
    TSharedPtr<FJsonObject> HandleGetAvailableMaterials(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleApplyMaterialToBlueprint(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintMaterialInfo(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleSetMeshAssetMaterial(const TSharedPtr<FJsonObject>& Params);

    // Blueprint analysis functions
    TSharedPtr<FJsonObject> HandleReadBlueprintContent(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAnalyzeBlueprintGraph(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintVariableDetails(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetBlueprintFunctionDetails(const TSharedPtr<FJsonObject>& Params);

    // Character blueprint creation
    TSharedPtr<FJsonObject> HandleCreateCharacterBlueprint(const TSharedPtr<FJsonObject>& Params);

    // Animation blueprint creation
    TSharedPtr<FJsonObject> HandleCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params);

    // AnimBP state machine setup
    TSharedPtr<FJsonObject> HandleSetupLocomotionStateMachine(const TSharedPtr<FJsonObject>& Params);

    // AnimBP BlendSpace1D locomotion setup (replaces state machine with continuous blending)
    TSharedPtr<FJsonObject> HandleSetupBlendspaceLocomotion(const TSharedPtr<FJsonObject>& Params);

    // Update existing character blueprint properties (AnimBP, mesh, etc.)
    TSharedPtr<FJsonObject> HandleSetCharacterProperties(const TSharedPtr<FJsonObject>& Params);

    // Set root motion enable/disable on an AnimSequence asset.
    TSharedPtr<FJsonObject> HandleSetAnimSequenceRootMotion(const TSharedPtr<FJsonObject>& Params);

    // Set bAlwaysResetOnEntry on a specific state inside an AnimBlueprint state machine.
    TSharedPtr<FJsonObject> HandleSetAnimStateAlwaysResetOnEntry(const TSharedPtr<FJsonObject>& Params);

    // Set MaxTransitionsPerFrame on an AnimBlueprint state machine.
    TSharedPtr<FJsonObject> HandleSetStateMachineMaxTransitionsPerFrame(const TSharedPtr<FJsonObject>& Params);

};
