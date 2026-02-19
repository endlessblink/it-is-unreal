// Copyright Epic Games, Inc. All Rights Reserved.
// Material Graph Commands for UnrealMCP - Programmatic Material Expression Creation

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Handles material graph manipulation commands for creating and connecting
 * material expressions programmatically.
 *
 * Supported expression types:
 * - Constants: Constant, Constant2Vector, Constant3Vector, Constant4Vector
 * - Parameters: ScalarParameter, VectorParameter, TextureParameter
 * - Math: Add, Subtract, Multiply, Divide, Power, Abs, Clamp, Lerp, OneMinus
 * - Texture: TextureSample, TextureCoordinate, Panner, Rotator
 * - Utility: WorldPosition, ObjectPosition, VertexColor, Time, ComponentMask, Sine, Cosine
 */
class UNREALMCP_API FEpicUnrealMCPMaterialGraphCommands
{
public:
    FEpicUnrealMCPMaterialGraphCommands();
    ~FEpicUnrealMCPMaterialGraphCommands() = default;

    /**
     * Handle incoming material graph commands.
     * Routes to appropriate handler based on CommandType.
     */
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // ============================================
    // Material Creation/Loading
    // ============================================

    /**
     * Create a new material asset.
     * Params: name, path (optional, default /Game/Materials/)
     */
    TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);

    /**
     * Get material graph information - lists all expressions and connections.
     * Params: material_path
     */
    TSharedPtr<FJsonObject> HandleGetMaterialGraph(const TSharedPtr<FJsonObject>& Params);

    // ============================================
    // Expression Management
    // ============================================

    /**
     * Add a material expression to a material.
     * Params: material_path, expression_type, pos_x, pos_y, expression_params (dict)
     *
     * expression_type options:
     *   Constant, Constant2Vector, Constant3Vector, Constant4Vector,
     *   ScalarParameter, VectorParameter, TextureSampleParameter2D,
     *   TextureSample, TextureCoordinate,
     *   Add, Subtract, Multiply, Divide, Power, Abs, Clamp, OneMinus,
     *   LinearInterpolate (Lerp),
     *   Sine, Cosine,
     *   WorldPosition, ObjectPosition, VertexColor, Time,
     *   ComponentMask, AppendVector
     */
    TSharedPtr<FJsonObject> HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params);

    /**
     * Connect two material expressions together.
     * Params: material_path, source_expression_id, output_index, target_expression_id, input_name
     */
    TSharedPtr<FJsonObject> HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params);

    /**
     * Connect an expression to a material output property (BaseColor, Roughness, etc.)
     * Params: material_path, expression_id, output_index, material_property
     *
     * material_property options:
     *   BaseColor, Metallic, Specular, Roughness, Anisotropy,
     *   EmissiveColor, Opacity, OpacityMask, Normal, Tangent,
     *   WorldPositionOffset, SubsurfaceColor, AmbientOcclusion
     */
    TSharedPtr<FJsonObject> HandleConnectToMaterialOutput(const TSharedPtr<FJsonObject>& Params);

    /**
     * Set a property on a material expression.
     * Params: material_path, expression_id, property_name, value
     */
    TSharedPtr<FJsonObject> HandleSetMaterialExpressionProperty(const TSharedPtr<FJsonObject>& Params);

    /**
     * Delete a material expression from a material.
     * Params: material_path, expression_id
     */
    TSharedPtr<FJsonObject> HandleDeleteMaterialExpression(const TSharedPtr<FJsonObject>& Params);

    /**
     * Recompile a material after making changes.
     * Params: material_path
     */
    TSharedPtr<FJsonObject> HandleRecompileMaterial(const TSharedPtr<FJsonObject>& Params);

    /**
     * Configure layers in a LandscapeLayerBlend material expression.
     * Params: material_path, expression_id, layers (array of {name, blend_type, preview_weight})
     */
    TSharedPtr<FJsonObject> HandleConfigureLandscapeLayerBlend(const TSharedPtr<FJsonObject>& Params);

    // ============================================
    // Helper Functions
    // ============================================

    /** Load a material by path */
    class UMaterial* LoadMaterial(const FString& MaterialPath);

    /** Find an expression by ID in a material */
    class UMaterialExpression* FindExpressionById(class UMaterial* Material, const FString& ExpressionId);

    /** Create expression of specified type */
    class UMaterialExpression* CreateExpression(class UMaterial* Material, const FString& ExpressionType, float PosX, float PosY);

    /** Get an expression's input by name */
    struct FExpressionInput* GetExpressionInput(class UMaterialExpression* Expression, const FString& InputName);

    /** Convert expression to JSON for inspection */
    TSharedPtr<FJsonObject> ExpressionToJson(class UMaterialExpression* Expression);

    /** Apply expression-specific parameters */
    bool ApplyExpressionParams(class UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Params);

    /** Expression counter for unique IDs */
    int32 ExpressionCounter;
};
