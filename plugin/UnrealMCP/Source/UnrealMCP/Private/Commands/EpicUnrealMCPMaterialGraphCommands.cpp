// Copyright Epic Games, Inc. All Rights Reserved.
// Material Graph Commands Implementation

#include "Commands/EpicUnrealMCPMaterialGraphCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialFactoryNew.h"

// Material includes
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceConstant.h"

// Material Expression includes
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionCosine.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionObjectPositionWS.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionRotator.h"
#include "LandscapeLayerInfoObject.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Materials/MaterialExpressionLandscapeLayerCoords.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"
#include "Materials/MaterialExpressionPerInstanceRandom.h"
#include "UObject/SavePackage.h"

FEpicUnrealMCPMaterialGraphCommands::FEpicUnrealMCPMaterialGraphCommands()
    : ExpressionCounter(0)
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_material_asset"))
    {
        return HandleCreateMaterial(Params);
    }
    else if (CommandType == TEXT("get_material_graph"))
    {
        return HandleGetMaterialGraph(Params);
    }
    else if (CommandType == TEXT("add_material_expression"))
    {
        return HandleAddMaterialExpression(Params);
    }
    else if (CommandType == TEXT("connect_material_expressions"))
    {
        return HandleConnectMaterialExpressions(Params);
    }
    else if (CommandType == TEXT("connect_to_material_output"))
    {
        return HandleConnectToMaterialOutput(Params);
    }
    else if (CommandType == TEXT("set_material_expression_property"))
    {
        return HandleSetMaterialExpressionProperty(Params);
    }
    else if (CommandType == TEXT("delete_material_expression"))
    {
        return HandleDeleteMaterialExpression(Params);
    }
    else if (CommandType == TEXT("recompile_material"))
    {
        return HandleRecompileMaterial(Params);
    }
    else if (CommandType == TEXT("configure_landscape_layer_blend"))
    {
        return HandleConfigureLandscapeLayerBlend(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material graph command: %s"), *CommandType));
}

// ============================================
// Helper Functions
// ============================================

UMaterial* FEpicUnrealMCPMaterialGraphCommands::LoadMaterial(const FString& MaterialPath)
{
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(MaterialPath);
    return Cast<UMaterial>(LoadedAsset);
}

UMaterialExpression* FEpicUnrealMCPMaterialGraphCommands::FindExpressionById(UMaterial* Material, const FString& ExpressionId)
{
    if (!Material) return nullptr;

    for (UMaterialExpression* Expr : Material->GetExpressions())
    {
        if (Expr && Expr->GetName() == ExpressionId)
        {
            return Expr;
        }
    }

    // Also check by description field (fallback)
    for (UMaterialExpression* Expr : Material->GetExpressions())
    {
        if (Expr && Expr->Desc.Contains(ExpressionId))
        {
            return Expr;
        }
    }

    return nullptr;
}

UMaterialExpression* FEpicUnrealMCPMaterialGraphCommands::CreateExpression(UMaterial* Material, const FString& ExpressionType, float PosX, float PosY)
{
    UMaterialExpression* NewExpression = nullptr;

    // Constants
    if (ExpressionType == TEXT("Constant"))
    {
        NewExpression = NewObject<UMaterialExpressionConstant>(Material);
    }
    else if (ExpressionType == TEXT("Constant2Vector"))
    {
        NewExpression = NewObject<UMaterialExpressionConstant2Vector>(Material);
    }
    else if (ExpressionType == TEXT("Constant3Vector"))
    {
        NewExpression = NewObject<UMaterialExpressionConstant3Vector>(Material);
    }
    else if (ExpressionType == TEXT("Constant4Vector"))
    {
        NewExpression = NewObject<UMaterialExpressionConstant4Vector>(Material);
    }
    // Parameters
    else if (ExpressionType == TEXT("ScalarParameter"))
    {
        NewExpression = NewObject<UMaterialExpressionScalarParameter>(Material);
    }
    else if (ExpressionType == TEXT("VectorParameter"))
    {
        NewExpression = NewObject<UMaterialExpressionVectorParameter>(Material);
    }
    else if (ExpressionType == TEXT("TextureSampleParameter2D") || ExpressionType == TEXT("TextureParameter"))
    {
        NewExpression = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
    }
    // Texture
    else if (ExpressionType == TEXT("TextureSample"))
    {
        NewExpression = NewObject<UMaterialExpressionTextureSample>(Material);
    }
    else if (ExpressionType == TEXT("TextureCoordinate") || ExpressionType == TEXT("TexCoord"))
    {
        NewExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
    }
    else if (ExpressionType == TEXT("Panner"))
    {
        NewExpression = NewObject<UMaterialExpressionPanner>(Material);
    }
    else if (ExpressionType == TEXT("Rotator"))
    {
        NewExpression = NewObject<UMaterialExpressionRotator>(Material);
    }
    // Landscape
    else if (ExpressionType == TEXT("LandscapeLayerBlend"))
    {
        NewExpression = NewObject<UMaterialExpressionLandscapeLayerBlend>(Material);
    }
    else if (ExpressionType == TEXT("LandscapeLayerCoords") || ExpressionType == TEXT("LandscapeCoords"))
    {
        NewExpression = NewObject<UMaterialExpressionLandscapeLayerCoords>(Material);
    }
    // Normal vectors
    else if (ExpressionType == TEXT("VertexNormalWS"))
    {
        NewExpression = NewObject<UMaterialExpressionVertexNormalWS>(Material);
    }
    else if (ExpressionType == TEXT("PixelNormalWS"))
    {
        NewExpression = NewObject<UMaterialExpressionPixelNormalWS>(Material);
    }
    // Additional math
    else if (ExpressionType == TEXT("DotProduct") || ExpressionType == TEXT("Dot"))
    {
        NewExpression = NewObject<UMaterialExpressionDotProduct>(Material);
    }
    else if (ExpressionType == TEXT("Saturate"))
    {
        NewExpression = NewObject<UMaterialExpressionSaturate>(Material);
    }
    // Math
    else if (ExpressionType == TEXT("Add"))
    {
        NewExpression = NewObject<UMaterialExpressionAdd>(Material);
    }
    else if (ExpressionType == TEXT("Subtract"))
    {
        NewExpression = NewObject<UMaterialExpressionSubtract>(Material);
    }
    else if (ExpressionType == TEXT("Multiply"))
    {
        NewExpression = NewObject<UMaterialExpressionMultiply>(Material);
    }
    else if (ExpressionType == TEXT("Divide"))
    {
        NewExpression = NewObject<UMaterialExpressionDivide>(Material);
    }
    else if (ExpressionType == TEXT("Power"))
    {
        NewExpression = NewObject<UMaterialExpressionPower>(Material);
    }
    else if (ExpressionType == TEXT("Abs"))
    {
        NewExpression = NewObject<UMaterialExpressionAbs>(Material);
    }
    else if (ExpressionType == TEXT("Clamp"))
    {
        NewExpression = NewObject<UMaterialExpressionClamp>(Material);
    }
    else if (ExpressionType == TEXT("OneMinus"))
    {
        NewExpression = NewObject<UMaterialExpressionOneMinus>(Material);
    }
    else if (ExpressionType == TEXT("LinearInterpolate") || ExpressionType == TEXT("Lerp"))
    {
        NewExpression = NewObject<UMaterialExpressionLinearInterpolate>(Material);
    }
    // Trigonometry
    else if (ExpressionType == TEXT("Sine"))
    {
        NewExpression = NewObject<UMaterialExpressionSine>(Material);
    }
    else if (ExpressionType == TEXT("Cosine"))
    {
        NewExpression = NewObject<UMaterialExpressionCosine>(Material);
    }
    // Utility
    else if (ExpressionType == TEXT("WorldPosition"))
    {
        NewExpression = NewObject<UMaterialExpressionWorldPosition>(Material);
    }
    else if (ExpressionType == TEXT("ObjectPosition"))
    {
        NewExpression = NewObject<UMaterialExpressionObjectPositionWS>(Material);
    }
    else if (ExpressionType == TEXT("VertexColor"))
    {
        NewExpression = NewObject<UMaterialExpressionVertexColor>(Material);
    }
    else if (ExpressionType == TEXT("Time"))
    {
        NewExpression = NewObject<UMaterialExpressionTime>(Material);
    }
    else if (ExpressionType == TEXT("ComponentMask"))
    {
        NewExpression = NewObject<UMaterialExpressionComponentMask>(Material);
    }
    else if (ExpressionType == TEXT("AppendVector") || ExpressionType == TEXT("Append"))
    {
        NewExpression = NewObject<UMaterialExpressionAppendVector>(Material);
    }
    else if (ExpressionType == TEXT("TwoSidedSign"))
    {
        NewExpression = NewObject<UMaterialExpressionTwoSidedSign>(Material);
    }
    else if (ExpressionType == TEXT("Noise"))
    {
        NewExpression = NewObject<UMaterialExpressionNoise>(Material);
    }
    else if (ExpressionType == TEXT("PerInstanceRandom"))
    {
        NewExpression = NewObject<UMaterialExpressionPerInstanceRandom>(Material);
    }

    if (NewExpression)
    {
        NewExpression->MaterialExpressionEditorX = PosX;
        NewExpression->MaterialExpressionEditorY = PosY;
        Material->GetExpressionCollection().AddExpression(NewExpression);
    }

    return NewExpression;
}

FExpressionInput* FEpicUnrealMCPMaterialGraphCommands::GetExpressionInput(UMaterialExpression* Expression, const FString& InputName)
{
    if (!Expression) return nullptr;

    // Common inputs by name
    FString LowerName = InputName.ToLower();

    // Check for Lerp/LinearInterpolate inputs
    if (UMaterialExpressionLinearInterpolate* Lerp = Cast<UMaterialExpressionLinearInterpolate>(Expression))
    {
        if (LowerName == TEXT("a")) return &Lerp->A;
        if (LowerName == TEXT("b")) return &Lerp->B;
        if (LowerName == TEXT("alpha")) return &Lerp->Alpha;
    }

    // Math operations (Add, Multiply, etc.)
    if (UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(Expression))
    {
        if (LowerName == TEXT("a")) return &Add->A;
        if (LowerName == TEXT("b")) return &Add->B;
    }
    if (UMaterialExpressionSubtract* Sub = Cast<UMaterialExpressionSubtract>(Expression))
    {
        if (LowerName == TEXT("a")) return &Sub->A;
        if (LowerName == TEXT("b")) return &Sub->B;
    }
    if (UMaterialExpressionMultiply* Mul = Cast<UMaterialExpressionMultiply>(Expression))
    {
        if (LowerName == TEXT("a")) return &Mul->A;
        if (LowerName == TEXT("b")) return &Mul->B;
    }
    if (UMaterialExpressionDivide* Div = Cast<UMaterialExpressionDivide>(Expression))
    {
        if (LowerName == TEXT("a")) return &Div->A;
        if (LowerName == TEXT("b")) return &Div->B;
    }
    if (UMaterialExpressionPower* Pow = Cast<UMaterialExpressionPower>(Expression))
    {
        if (LowerName == TEXT("base")) return &Pow->Base;
        if (LowerName == TEXT("exponent") || LowerName == TEXT("exp")) return &Pow->Exponent;
    }

    // Clamp
    if (UMaterialExpressionClamp* Clamp = Cast<UMaterialExpressionClamp>(Expression))
    {
        if (LowerName == TEXT("input")) return &Clamp->Input;
        if (LowerName == TEXT("min")) return &Clamp->Min;
        if (LowerName == TEXT("max")) return &Clamp->Max;
    }

    // Sine/Cosine
    if (UMaterialExpressionSine* Sine = Cast<UMaterialExpressionSine>(Expression))
    {
        if (LowerName == TEXT("input")) return &Sine->Input;
    }
    if (UMaterialExpressionCosine* Cosine = Cast<UMaterialExpressionCosine>(Expression))
    {
        if (LowerName == TEXT("input")) return &Cosine->Input;
    }

    // OneMinus
    if (UMaterialExpressionOneMinus* OneMinus = Cast<UMaterialExpressionOneMinus>(Expression))
    {
        if (LowerName == TEXT("input")) return &OneMinus->Input;
    }

    // Abs
    if (UMaterialExpressionAbs* Abs = Cast<UMaterialExpressionAbs>(Expression))
    {
        if (LowerName == TEXT("input")) return &Abs->Input;
    }

    // ComponentMask
    if (UMaterialExpressionComponentMask* Mask = Cast<UMaterialExpressionComponentMask>(Expression))
    {
        if (LowerName == TEXT("input")) return &Mask->Input;
    }

    // AppendVector
    if (UMaterialExpressionAppendVector* Append = Cast<UMaterialExpressionAppendVector>(Expression))
    {
        if (LowerName == TEXT("a")) return &Append->A;
        if (LowerName == TEXT("b")) return &Append->B;
    }

    // TextureSample
    if (UMaterialExpressionTextureSample* TexSample = Cast<UMaterialExpressionTextureSample>(Expression))
    {
        if (LowerName == TEXT("coordinates") || LowerName == TEXT("uvs") || LowerName == TEXT("uv"))
        {
            return &TexSample->Coordinates;
        }
    }

    // Panner
    if (UMaterialExpressionPanner* Panner = Cast<UMaterialExpressionPanner>(Expression))
    {
        if (LowerName == TEXT("coordinate") || LowerName == TEXT("uv")) return &Panner->Coordinate;
        if (LowerName == TEXT("time")) return &Panner->Time;
        if (LowerName == TEXT("speed")) return &Panner->Speed;
    }

    // LandscapeLayerBlend - dynamic inputs based on layer names
    if (UMaterialExpressionLandscapeLayerBlend* LayerBlend = Cast<UMaterialExpressionLandscapeLayerBlend>(Expression))
    {
        // InputName should match a layer name (e.g., "Layer_0", "Layer_1", "Grass", "Dirt")
        for (int32 i = 0; i < LayerBlend->Layers.Num(); i++)
        {
            if (LayerBlend->Layers[i].LayerName.ToString().ToLower() == LowerName ||
                LayerBlend->Layers[i].LayerName.ToString() == InputName)
            {
                return &LayerBlend->Layers[i].LayerInput;
            }
        }
        // Also support "height_<layername>" for height input
        if (LowerName.StartsWith(TEXT("height_")))
        {
            FString LayerNamePart = InputName.Mid(7); // Remove "height_"
            for (int32 i = 0; i < LayerBlend->Layers.Num(); i++)
            {
                if (LayerBlend->Layers[i].LayerName.ToString().ToLower() == LayerNamePart.ToLower() ||
                    LayerBlend->Layers[i].LayerName.ToString() == LayerNamePart)
                {
                    return &LayerBlend->Layers[i].HeightInput;
                }
            }
        }
    }

    // DotProduct
    if (UMaterialExpressionDotProduct* DotProd = Cast<UMaterialExpressionDotProduct>(Expression))
    {
        if (LowerName == TEXT("a")) return &DotProd->A;
        if (LowerName == TEXT("b")) return &DotProd->B;
    }

    // Saturate
    if (UMaterialExpressionSaturate* Sat = Cast<UMaterialExpressionSaturate>(Expression))
    {
        if (LowerName == TEXT("input")) return &Sat->Input;
    }

    // Noise (explicit Cast)
    if (UMaterialExpressionNoise* NoiseExpr = Cast<UMaterialExpressionNoise>(Expression))
    {
        if (LowerName == TEXT("position") || LowerName == TEXT("pos")) return &NoiseExpr->Position;
        if (LowerName == TEXT("filterwidth") || LowerName == TEXT("filter_width")) return &NoiseExpr->FilterWidth;
    }

    // Generic fallback: iterate all inputs by name using UE reflection.
    // GetInput(i) returns nullptr when index is out of range.
    // Handles any expression type not explicitly listed above.
    for (int32 i = 0; ; i++)
    {
        FExpressionInput* Input = Expression->GetInput(i);
        if (!Input) break;
        FName InputFName = Expression->GetInputName(i);
        if (InputFName.ToString().ToLower() == LowerName)
        {
            return Input;
        }
    }

    return nullptr;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::ExpressionToJson(UMaterialExpression* Expression)
{
    TSharedPtr<FJsonObject> ExprObj = MakeShared<FJsonObject>();
    ExprObj->SetStringField(TEXT("id"), Expression->GetName());
    ExprObj->SetStringField(TEXT("type"), Expression->GetClass()->GetName());
    ExprObj->SetNumberField(TEXT("pos_x"), Expression->MaterialExpressionEditorX);
    ExprObj->SetNumberField(TEXT("pos_y"), Expression->MaterialExpressionEditorY);
    ExprObj->SetStringField(TEXT("desc"), Expression->Desc);
    return ExprObj;
}

bool FEpicUnrealMCPMaterialGraphCommands::ApplyExpressionParams(UMaterialExpression* Expression, const TSharedPtr<FJsonObject>& Params)
{
    if (!Expression || !Params.IsValid()) return false;

    // Constant
    if (UMaterialExpressionConstant* Const = Cast<UMaterialExpressionConstant>(Expression))
    {
        if (Params->HasField(TEXT("value")) || Params->HasField(TEXT("r")))
        {
            Const->R = Params->HasField(TEXT("value")) ? Params->GetNumberField(TEXT("value")) : Params->GetNumberField(TEXT("r"));
        }
    }

    // Constant2Vector
    if (UMaterialExpressionConstant2Vector* Const2 = Cast<UMaterialExpressionConstant2Vector>(Expression))
    {
        if (Params->HasField(TEXT("r"))) Const2->R = Params->GetNumberField(TEXT("r"));
        if (Params->HasField(TEXT("g"))) Const2->G = Params->GetNumberField(TEXT("g"));
    }

    // Constant3Vector
    if (UMaterialExpressionConstant3Vector* Const3 = Cast<UMaterialExpressionConstant3Vector>(Expression))
    {
        if (Params->HasField(TEXT("r")) && Params->HasField(TEXT("g")) && Params->HasField(TEXT("b")))
        {
            Const3->Constant = FLinearColor(
                Params->GetNumberField(TEXT("r")),
                Params->GetNumberField(TEXT("g")),
                Params->GetNumberField(TEXT("b"))
            );
        }
        if (Params->HasField(TEXT("color")))
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
            {
                Const3->Constant = FLinearColor(
                    (*ColorArray)[0]->AsNumber(),
                    (*ColorArray)[1]->AsNumber(),
                    (*ColorArray)[2]->AsNumber()
                );
            }
        }
    }

    // ScalarParameter
    if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expression))
    {
        if (Params->HasField(TEXT("parameter_name")))
        {
            ScalarParam->ParameterName = FName(*Params->GetStringField(TEXT("parameter_name")));
        }
        if (Params->HasField(TEXT("default_value")))
        {
            ScalarParam->DefaultValue = Params->GetNumberField(TEXT("default_value"));
        }
    }

    // VectorParameter
    if (UMaterialExpressionVectorParameter* VecParam = Cast<UMaterialExpressionVectorParameter>(Expression))
    {
        if (Params->HasField(TEXT("parameter_name")))
        {
            VecParam->ParameterName = FName(*Params->GetStringField(TEXT("parameter_name")));
        }
        if (Params->HasField(TEXT("default_value")))
        {
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if (Params->TryGetArrayField(TEXT("default_value"), ColorArray) && ColorArray->Num() >= 3)
            {
                VecParam->DefaultValue = FLinearColor(
                    (*ColorArray)[0]->AsNumber(),
                    (*ColorArray)[1]->AsNumber(),
                    (*ColorArray)[2]->AsNumber(),
                    ColorArray->Num() >= 4 ? (*ColorArray)[3]->AsNumber() : 1.0f
                );
            }
        }
    }

    // TextureSampleParameter2D
    if (UMaterialExpressionTextureSampleParameter2D* TexParam = Cast<UMaterialExpressionTextureSampleParameter2D>(Expression))
    {
        if (Params->HasField(TEXT("parameter_name")))
        {
            TexParam->ParameterName = FName(*Params->GetStringField(TEXT("parameter_name")));
        }
        if (Params->HasField(TEXT("texture_path")))
        {
            FString TexturePath = Params->GetStringField(TEXT("texture_path"));
            UTexture* Texture = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(TexturePath));
            if (Texture)
            {
                TexParam->Texture = Texture;
            }
        }
    }

    // TextureSample
    if (UMaterialExpressionTextureSample* TexSample = Cast<UMaterialExpressionTextureSample>(Expression))
    {
        if (Params->HasField(TEXT("texture_path")))
        {
            FString TexturePath = Params->GetStringField(TEXT("texture_path"));
            UTexture* Texture = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(TexturePath));
            if (Texture)
            {
                TexSample->Texture = Texture;
            }
        }
        if (Params->HasField(TEXT("sampler_type")))
        {
            FString SamplerTypeStr = Params->GetStringField(TEXT("sampler_type"));
            if (SamplerTypeStr == TEXT("Color"))
                TexSample->SamplerType = SAMPLERTYPE_Color;
            else if (SamplerTypeStr == TEXT("Normal"))
                TexSample->SamplerType = SAMPLERTYPE_Normal;
            else if (SamplerTypeStr == TEXT("Masks"))
                TexSample->SamplerType = SAMPLERTYPE_Masks;
            else if (SamplerTypeStr == TEXT("LinearColor"))
                TexSample->SamplerType = SAMPLERTYPE_LinearColor;
            else if (SamplerTypeStr == TEXT("Grayscale"))
                TexSample->SamplerType = SAMPLERTYPE_Grayscale;
        }
    }

    // TextureCoordinate
    if (UMaterialExpressionTextureCoordinate* TexCoord = Cast<UMaterialExpressionTextureCoordinate>(Expression))
    {
        if (Params->HasField(TEXT("u_tiling"))) TexCoord->UTiling = Params->GetNumberField(TEXT("u_tiling"));
        if (Params->HasField(TEXT("v_tiling"))) TexCoord->VTiling = Params->GetNumberField(TEXT("v_tiling"));
        if (Params->HasField(TEXT("coordinate_index"))) TexCoord->CoordinateIndex = Params->GetIntegerField(TEXT("coordinate_index"));
    }

    // ComponentMask
    if (UMaterialExpressionComponentMask* Mask = Cast<UMaterialExpressionComponentMask>(Expression))
    {
        if (Params->HasField(TEXT("r"))) Mask->R = Params->GetBoolField(TEXT("r"));
        if (Params->HasField(TEXT("g"))) Mask->G = Params->GetBoolField(TEXT("g"));
        if (Params->HasField(TEXT("b"))) Mask->B = Params->GetBoolField(TEXT("b"));
        if (Params->HasField(TEXT("a"))) Mask->A = Params->GetBoolField(TEXT("a"));
    }

    // Add constant value for Math nodes
    if (UMaterialExpressionAdd* Add = Cast<UMaterialExpressionAdd>(Expression))
    {
        if (Params->HasField(TEXT("const_a"))) Add->ConstA = Params->GetNumberField(TEXT("const_a"));
        if (Params->HasField(TEXT("const_b"))) Add->ConstB = Params->GetNumberField(TEXT("const_b"));
    }
    if (UMaterialExpressionMultiply* Mul = Cast<UMaterialExpressionMultiply>(Expression))
    {
        if (Params->HasField(TEXT("const_a"))) Mul->ConstA = Params->GetNumberField(TEXT("const_a"));
        if (Params->HasField(TEXT("const_b"))) Mul->ConstB = Params->GetNumberField(TEXT("const_b"));
    }
    if (UMaterialExpressionDivide* Div = Cast<UMaterialExpressionDivide>(Expression))
    {
        if (Params->HasField(TEXT("const_a"))) Div->ConstA = Params->GetNumberField(TEXT("const_a"));
        if (Params->HasField(TEXT("const_b"))) Div->ConstB = Params->GetNumberField(TEXT("const_b"));
    }

    // Lerp constant values
    if (UMaterialExpressionLinearInterpolate* Lerp = Cast<UMaterialExpressionLinearInterpolate>(Expression))
    {
        if (Params->HasField(TEXT("const_a"))) Lerp->ConstA = Params->GetNumberField(TEXT("const_a"));
        if (Params->HasField(TEXT("const_b"))) Lerp->ConstB = Params->GetNumberField(TEXT("const_b"));
        if (Params->HasField(TEXT("const_alpha"))) Lerp->ConstAlpha = Params->GetNumberField(TEXT("const_alpha"));
    }

    // Panner
    if (UMaterialExpressionPanner* Panner = Cast<UMaterialExpressionPanner>(Expression))
    {
        if (Params->HasField(TEXT("speed_x"))) Panner->SpeedX = Params->GetNumberField(TEXT("speed_x"));
        if (Params->HasField(TEXT("speed_y"))) Panner->SpeedY = Params->GetNumberField(TEXT("speed_y"));
    }

    // LandscapeLayerCoords
    if (UMaterialExpressionLandscapeLayerCoords* LandCoords = Cast<UMaterialExpressionLandscapeLayerCoords>(Expression))
    {
        if (Params->HasField(TEXT("mapping_type")))
        {
            FString MappingTypeStr = Params->GetStringField(TEXT("mapping_type"));
            if (MappingTypeStr == TEXT("XY"))
                LandCoords->MappingType = ETerrainCoordMappingType::TCMT_XY;
            else if (MappingTypeStr == TEXT("XZ"))
                LandCoords->MappingType = ETerrainCoordMappingType::TCMT_XZ;
            else if (MappingTypeStr == TEXT("YZ"))
                LandCoords->MappingType = ETerrainCoordMappingType::TCMT_YZ;
        }
        if (Params->HasField(TEXT("mapping_scale"))) LandCoords->MappingScale = Params->GetNumberField(TEXT("mapping_scale"));
        if (Params->HasField(TEXT("mapping_rotation"))) LandCoords->MappingRotation = Params->GetNumberField(TEXT("mapping_rotation"));
        if (Params->HasField(TEXT("mapping_pan_u"))) LandCoords->MappingPanU = Params->GetNumberField(TEXT("mapping_pan_u"));
        if (Params->HasField(TEXT("mapping_pan_v"))) LandCoords->MappingPanV = Params->GetNumberField(TEXT("mapping_pan_v"));
    }

    // Power - const exponent fallback
    if (UMaterialExpressionPower* Pow = Cast<UMaterialExpressionPower>(Expression))
    {
        if (Params->HasField(TEXT("const_exponent"))) Pow->ConstExponent = Params->GetNumberField(TEXT("const_exponent"));
    }

    // Noise
    if (UMaterialExpressionNoise* NoiseExpr = Cast<UMaterialExpressionNoise>(Expression))
    {
        if (Params->HasField(TEXT("scale"))) NoiseExpr->Scale = Params->GetNumberField(TEXT("scale"));
        if (Params->HasField(TEXT("quality"))) NoiseExpr->Quality = Params->GetIntegerField(TEXT("quality"));
        if (Params->HasField(TEXT("levels"))) NoiseExpr->Levels = Params->GetIntegerField(TEXT("levels"));
        if (Params->HasField(TEXT("output_min"))) NoiseExpr->OutputMin = Params->GetNumberField(TEXT("output_min"));
        if (Params->HasField(TEXT("output_max"))) NoiseExpr->OutputMax = Params->GetNumberField(TEXT("output_max"));
    }

    return true;
}

// ============================================
// Command Handlers
// ============================================

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialName;
    if (!Params->TryGetStringField(TEXT("name"), MaterialName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString MaterialPath = TEXT("/Game/Materials/");
    Params->TryGetStringField(TEXT("path"), MaterialPath);

    // Check if material already exists
    FString FullPath = MaterialPath + MaterialName;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material already exists: %s"), *FullPath));
    }

    // Create the material
    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    UPackage* Package = CreatePackage(*(MaterialPath + MaterialName));
    UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
        UMaterial::StaticClass(), Package, *MaterialName,
        RF_Standalone | RF_Public, nullptr, GWarn));

    if (!NewMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
    }

    // Register with asset registry
    FAssetRegistryModule::AssetCreated(NewMaterial);
    Package->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), MaterialName);
    ResultObj->SetStringField(TEXT("path"), FullPath);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleGetMaterialGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    TArray<TSharedPtr<FJsonValue>> ExpressionArray;
    for (UMaterialExpression* Expr : Material->GetExpressions())
    {
        if (Expr)
        {
            ExpressionArray.Add(MakeShared<FJsonValueObject>(ExpressionToJson(Expr)));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetArrayField(TEXT("expressions"), ExpressionArray);
    ResultObj->SetNumberField(TEXT("expression_count"), ExpressionArray.Num());
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleAddMaterialExpression(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionType;
    if (!Params->TryGetStringField(TEXT("expression_type"), ExpressionType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_type' parameter"));
    }

    float PosX = 0.0f, PosY = 0.0f;
    Params->TryGetNumberField(TEXT("pos_x"), PosX);
    Params->TryGetNumberField(TEXT("pos_y"), PosY);

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Pre-edit notification
    Material->PreEditChange(nullptr);

    UMaterialExpression* NewExpression = CreateExpression(Material, ExpressionType, PosX, PosY);
    if (!NewExpression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown expression type: %s"), *ExpressionType));
    }

    // Store unique ID in description for reliable lookup
    FString UniqueId = FString::Printf(TEXT("MCP_%d"), ++ExpressionCounter);
    NewExpression->Desc = UniqueId;

    // Apply expression-specific parameters
    const TSharedPtr<FJsonObject>* ExpressionParams;
    if (Params->TryGetObjectField(TEXT("expression_params"), ExpressionParams))
    {
        ApplyExpressionParams(NewExpression, *ExpressionParams);
    }
    else
    {
        // Try to apply params from the root Params object
        ApplyExpressionParams(NewExpression, Params);
    }

    // Post-edit notification
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("expression_id"), NewExpression->GetName());
    ResultObj->SetStringField(TEXT("mcp_id"), UniqueId);
    ResultObj->SetStringField(TEXT("expression_type"), ExpressionType);
    ResultObj->SetNumberField(TEXT("pos_x"), PosX);
    ResultObj->SetNumberField(TEXT("pos_y"), PosY);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleConnectMaterialExpressions(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString SourceExpressionId, TargetExpressionId;
    if (!Params->TryGetStringField(TEXT("source_expression_id"), SourceExpressionId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_expression_id' parameter"));
    }
    if (!Params->TryGetStringField(TEXT("target_expression_id"), TargetExpressionId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_expression_id' parameter"));
    }

    int32 OutputIndex = 0;
    Params->TryGetNumberField(TEXT("output_index"), OutputIndex);

    FString InputName = TEXT("input");
    Params->TryGetStringField(TEXT("input_name"), InputName);

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    UMaterialExpression* SourceExpr = FindExpressionById(Material, SourceExpressionId);
    UMaterialExpression* TargetExpr = FindExpressionById(Material, TargetExpressionId);

    if (!SourceExpr)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source expression not found: %s"), *SourceExpressionId));
    }
    if (!TargetExpr)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Target expression not found: %s"), *TargetExpressionId));
    }

    FExpressionInput* Input = GetExpressionInput(TargetExpr, InputName);
    if (!Input)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Input '%s' not found on expression"), *InputName));
    }

    // Pre-edit notification
    Material->PreEditChange(nullptr);

    // Make the connection
    Input->Connect(OutputIndex, SourceExpr);

    // Post-edit notification
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("source_expression_id"), SourceExpressionId);
    ResultObj->SetStringField(TEXT("target_expression_id"), TargetExpressionId);
    ResultObj->SetNumberField(TEXT("output_index"), OutputIndex);
    ResultObj->SetStringField(TEXT("input_name"), InputName);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleConnectToMaterialOutput(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionId;
    if (!Params->TryGetStringField(TEXT("expression_id"), ExpressionId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }

    FString MaterialProperty;
    if (!Params->TryGetStringField(TEXT("material_property"), MaterialProperty))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_property' parameter"));
    }

    int32 OutputIndex = 0;
    Params->TryGetNumberField(TEXT("output_index"), OutputIndex);

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    UMaterialExpression* Expression = FindExpressionById(Material, ExpressionId);
    if (!Expression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Expression not found: %s"), *ExpressionId));
    }

    // Pre-edit notification
    Material->PreEditChange(nullptr);

    // Get the editor-only data for UE5
    UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
    if (!EditorData)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get material editor data"));
    }

    // Connect to the appropriate material property
    FString PropLower = MaterialProperty.ToLower();
    bool bConnected = false;

    if (PropLower == TEXT("basecolor") || PropLower == TEXT("base_color"))
    {
        EditorData->BaseColor.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("metallic"))
    {
        EditorData->Metallic.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("specular"))
    {
        EditorData->Specular.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("roughness"))
    {
        EditorData->Roughness.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("anisotropy"))
    {
        EditorData->Anisotropy.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("emissivecolor") || PropLower == TEXT("emissive_color") || PropLower == TEXT("emissive"))
    {
        EditorData->EmissiveColor.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("opacity"))
    {
        EditorData->Opacity.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("opacitymask") || PropLower == TEXT("opacity_mask"))
    {
        EditorData->OpacityMask.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("normal"))
    {
        EditorData->Normal.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("tangent"))
    {
        EditorData->Tangent.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("worldpositionoffset") || PropLower == TEXT("world_position_offset"))
    {
        EditorData->WorldPositionOffset.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("subsurfacecolor") || PropLower == TEXT("subsurface_color"))
    {
        EditorData->SubsurfaceColor.Connect(OutputIndex, Expression);
        bConnected = true;
    }
    else if (PropLower == TEXT("ambientocclusion") || PropLower == TEXT("ambient_occlusion") || PropLower == TEXT("ao"))
    {
        EditorData->AmbientOcclusion.Connect(OutputIndex, Expression);
        bConnected = true;
    }

    if (!bConnected)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material property: %s"), *MaterialProperty));
    }

    // Post-edit notification
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("expression_id"), ExpressionId);
    ResultObj->SetStringField(TEXT("material_property"), MaterialProperty);
    ResultObj->SetNumberField(TEXT("output_index"), OutputIndex);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleSetMaterialExpressionProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionId;
    if (!Params->TryGetStringField(TEXT("expression_id"), ExpressionId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Special case: expression_id="material" sets material-level properties
    if (ExpressionId == TEXT("material"))
    {
        Material->PreEditChange(nullptr);

        const TSharedPtr<FJsonObject>* PropertiesObj;
        TSharedPtr<FJsonObject> Props;
        if (Params->TryGetObjectField(TEXT("properties"), PropertiesObj))
        {
            Props = *PropertiesObj;
        }
        else
        {
            Props = Params;
        }

        if (Props->HasField(TEXT("two_sided")))
        {
            Material->TwoSided = Props->GetBoolField(TEXT("two_sided"));
        }

        Material->PostEditChange();
        Material->MarkPackageDirty();

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("expression_id"), TEXT("material"));
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    UMaterialExpression* Expression = FindExpressionById(Material, ExpressionId);
    if (!Expression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Expression not found: %s"), *ExpressionId));
    }

    // Pre-edit notification
    Material->PreEditChange(nullptr);

    // Apply the properties
    ApplyExpressionParams(Expression, Params);

    // Post-edit notification
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("expression_id"), ExpressionId);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleDeleteMaterialExpression(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionId;
    if (!Params->TryGetStringField(TEXT("expression_id"), ExpressionId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    UMaterialExpression* Expression = FindExpressionById(Material, ExpressionId);
    if (!Expression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Expression not found: %s"), *ExpressionId));
    }

    // Pre-edit notification
    Material->PreEditChange(nullptr);

    // Remove the expression
    Material->GetExpressionCollection().RemoveExpression(Expression);

    // Post-edit notification
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("deleted_expression_id"), ExpressionId);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleRecompileMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Force recompilation
    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    Material->MarkPackageDirty();

    // Save material package to disk so landscape components can create proper shader instances
    UPackage* Package = Material->GetOutermost();
    if (Package)
    {
        FString PackagePath = Package->GetName();
        FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

        // Ensure directory exists
        FString PackageDirectory = FPaths::GetPath(PackageFilename);
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.DirectoryExists(*PackageDirectory))
        {
            PlatformFile.CreateDirectoryTree(*PackageDirectory);
        }

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        bool bSaved = UPackage::SavePackage(Package, Material, *PackageFilename, SaveArgs);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
        ResultObj->SetBoolField(TEXT("recompiled"), true);
        ResultObj->SetBoolField(TEXT("saved"), bSaved);
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetBoolField(TEXT("recompiled"), true);
    ResultObj->SetBoolField(TEXT("saved"), false);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPMaterialGraphCommands::HandleConfigureLandscapeLayerBlend(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ExpressionId;
    if (!Params->TryGetStringField(TEXT("expression_id"), ExpressionId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'expression_id' parameter"));
    }

    const TArray<TSharedPtr<FJsonValue>>* LayersArray;
    if (!Params->TryGetArrayField(TEXT("layers"), LayersArray))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'layers' array parameter"));
    }

    UMaterial* Material = LoadMaterial(MaterialPath);
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    UMaterialExpression* Expression = FindExpressionById(Material, ExpressionId);
    if (!Expression)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Expression not found: %s"), *ExpressionId));
    }

    UMaterialExpressionLandscapeLayerBlend* LayerBlend = Cast<UMaterialExpressionLandscapeLayerBlend>(Expression);
    if (!LayerBlend)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Expression is not a LandscapeLayerBlend"));
    }

    // Clear existing layers
    LayerBlend->Layers.Empty();

    // Add new layers from the array
    for (const TSharedPtr<FJsonValue>& LayerValue : *LayersArray)
    {
        const TSharedPtr<FJsonObject>* LayerObj;
        if (LayerValue->TryGetObject(LayerObj))
        {
            FLayerBlendInput NewLayer;

            FString LayerName;
            if ((*LayerObj)->TryGetStringField(TEXT("name"), LayerName))
            {
                NewLayer.LayerName = FName(*LayerName);
            }

            FString BlendType;
            if ((*LayerObj)->TryGetStringField(TEXT("blend_type"), BlendType))
            {
                if (BlendType == TEXT("LB_AlphaBlend"))
                {
                    NewLayer.BlendType = LB_AlphaBlend;
                }
                else if (BlendType == TEXT("LB_HeightBlend"))
                {
                    NewLayer.BlendType = LB_HeightBlend;
                }
                else
                {
                    NewLayer.BlendType = LB_WeightBlend;
                }
            }
            else
            {
                NewLayer.BlendType = LB_WeightBlend;
            }

            double PreviewWeight = 0.0;
            if ((*LayerObj)->TryGetNumberField(TEXT("preview_weight"), PreviewWeight))
            {
                NewLayer.PreviewWeight = PreviewWeight;
            }

            // Set a default visible color if no texture is connected
            const TArray<TSharedPtr<FJsonValue>>* ColorArray;
            if ((*LayerObj)->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
            {
                NewLayer.ConstLayerInput = FVector(
                    (*ColorArray)[0]->AsNumber(),
                    (*ColorArray)[1]->AsNumber(),
                    (*ColorArray)[2]->AsNumber()
                );
            }
            else
            {
                // Default to gray so it's visible when not connected to texture
                NewLayer.ConstLayerInput = FVector(0.5, 0.5, 0.5);
            }

            // ConstHeightInput default for height blending
            NewLayer.ConstHeightInput = 0.0f;

            LayerBlend->Layers.Add(NewLayer);
        }
    }

    // Mark dirty and recompile
    Material->PreEditChange(nullptr);
    Material->PostEditChange();
    Material->MarkPackageDirty();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetStringField(TEXT("expression_id"), ExpressionId);
    ResultObj->SetNumberField(TEXT("layer_count"), LayerBlend->Layers.Num());
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}
