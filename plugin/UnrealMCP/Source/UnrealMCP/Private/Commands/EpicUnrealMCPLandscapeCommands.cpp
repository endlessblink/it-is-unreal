#include "Commands/EpicUnrealMCPLandscapeCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeInfo.h"
#include "LandscapeComponent.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeEdit.h"
#include "LandscapeEditorUtils.h"
#include "LandscapeDataAccess.h"
#include "LandscapeLayerInfoObject.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "UObject/SavePackage.h"

FEpicUnrealMCPLandscapeCommands::FEpicUnrealMCPLandscapeCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("get_landscape_info"))
    {
        return HandleGetLandscapeInfo(Params);
    }
    else if (CommandType == TEXT("sculpt_landscape"))
    {
        return HandleSculptLandscape(Params);
    }
    else if (CommandType == TEXT("smooth_landscape"))
    {
        return HandleSmoothLandscape(Params);
    }
    else if (CommandType == TEXT("flatten_landscape"))
    {
        return HandleFlattenLandscape(Params);
    }
    else if (CommandType == TEXT("paint_landscape_layer"))
    {
        return HandlePaintLandscapeLayer(Params);
    }
    else if (CommandType == TEXT("get_landscape_layers"))
    {
        return HandleGetLandscapeLayers(Params);
    }
    else if (CommandType == TEXT("set_landscape_material"))
    {
        return HandleSetLandscapeMaterial(Params);
    }
    else if (CommandType == TEXT("create_landscape_layer"))
    {
        return HandleCreateLandscapeLayer(Params);
    }
    else if (CommandType == TEXT("add_layer_to_landscape"))
    {
        return HandleAddLayerToLandscape(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown landscape command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleGetLandscapeInfo(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    TArray<TSharedPtr<FJsonValue>> LandscapeArray;

    // Find all landscape actors
    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            TSharedPtr<FJsonObject> LandscapeObj = MakeShared<FJsonObject>();
            LandscapeObj->SetStringField(TEXT("name"), LandscapeProxy->GetName());
            LandscapeObj->SetStringField(TEXT("class"), LandscapeProxy->GetClass()->GetName());

            // Get bounds
            FBox Bounds = LandscapeProxy->GetComponentsBoundingBox();
            TArray<TSharedPtr<FJsonValue>> MinArray;
            MinArray.Add(MakeShared<FJsonValueNumber>(Bounds.Min.X));
            MinArray.Add(MakeShared<FJsonValueNumber>(Bounds.Min.Y));
            MinArray.Add(MakeShared<FJsonValueNumber>(Bounds.Min.Z));
            LandscapeObj->SetArrayField(TEXT("bounds_min"), MinArray);

            TArray<TSharedPtr<FJsonValue>> MaxArray;
            MaxArray.Add(MakeShared<FJsonValueNumber>(Bounds.Max.X));
            MaxArray.Add(MakeShared<FJsonValueNumber>(Bounds.Max.Y));
            MaxArray.Add(MakeShared<FJsonValueNumber>(Bounds.Max.Z));
            LandscapeObj->SetArrayField(TEXT("bounds_max"), MaxArray);

            // Get component count
            TArray<ULandscapeComponent*> LandscapeComponents;
            LandscapeProxy->GetComponents<ULandscapeComponent>(LandscapeComponents);
            LandscapeObj->SetNumberField(TEXT("component_count"), LandscapeComponents.Num());

            // Get scale
            FVector Scale = LandscapeProxy->GetActorScale3D();
            TArray<TSharedPtr<FJsonValue>> ScaleArray;
            ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
            ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
            ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
            LandscapeObj->SetArrayField(TEXT("scale"), ScaleArray);

            // Get material
            UMaterialInterface* LandscapeMaterial = LandscapeProxy->GetLandscapeMaterial();
            if (LandscapeMaterial)
            {
                LandscapeObj->SetStringField(TEXT("material"), LandscapeMaterial->GetPathName());
            }

            // Get layer info from LandscapeInfo
            TArray<TSharedPtr<FJsonValue>> LayersArray;
            ULandscapeInfo* LandscapeInfoRef = LandscapeProxy->GetLandscapeInfo();
            if (LandscapeInfoRef)
            {
                for (const FLandscapeInfoLayerSettings& LayerSettings : LandscapeInfoRef->Layers)
                {
                    if (LayerSettings.LayerInfoObj)
                    {
                        TSharedPtr<FJsonObject> LayerObj = MakeShared<FJsonObject>();
                        LayerObj->SetStringField(TEXT("name"), LayerSettings.LayerInfoObj->GetLayerName().ToString());
                        LayerObj->SetStringField(TEXT("path"), LayerSettings.LayerInfoObj->GetPathName());
                        LayersArray.Add(MakeShared<FJsonValueObject>(LayerObj));
                    }
                }
            }
            LandscapeObj->SetArrayField(TEXT("layers"), LayersArray);

            LandscapeArray.Add(MakeShared<FJsonValueObject>(LandscapeObj));
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetArrayField(TEXT("landscapes"), LandscapeArray);
    Result->SetNumberField(TEXT("count"), LandscapeArray.Num());

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleSculptLandscape(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters
    FVector Location(0, 0, 0);
    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'location' parameter"));
    }

    float Radius = 500.0f;
    if (Params->HasField(TEXT("radius")))
    {
        Radius = Params->GetNumberField(TEXT("radius"));
    }

    float Strength = 0.5f;
    if (Params->HasField(TEXT("strength")))
    {
        Strength = Params->GetNumberField(TEXT("strength"));
    }

    bool bRaise = true;
    if (Params->HasField(TEXT("raise")))
    {
        bRaise = Params->GetBoolField(TEXT("raise"));
    }

    float Falloff = 0.5f;
    if (Params->HasField(TEXT("falloff")))
    {
        Falloff = Params->GetNumberField(TEXT("falloff"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Find landscape at location
    ALandscapeProxy* TargetLandscape = nullptr;
    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            FBox Bounds = LandscapeProxy->GetComponentsBoundingBox();
            // Check if XY location is within landscape bounds
            if (Location.X >= Bounds.Min.X && Location.X <= Bounds.Max.X &&
                Location.Y >= Bounds.Min.Y && Location.Y <= Bounds.Max.Y)
            {
                TargetLandscape = LandscapeProxy;
                break;
            }
        }
    }

    if (!TargetLandscape)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found at specified location"));
    }

    // Get landscape info
    ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
    if (!LandscapeInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get landscape info"));
    }

    // Convert world location to landscape coordinates
    FVector LandscapeLocation = TargetLandscape->GetActorLocation();
    FVector LandscapeScale = TargetLandscape->GetActorScale3D();

    int32 CenterX = FMath::RoundToInt((Location.X - LandscapeLocation.X) / LandscapeScale.X);
    int32 CenterY = FMath::RoundToInt((Location.Y - LandscapeLocation.Y) / LandscapeScale.Y);
    int32 RadiusInQuads = FMath::RoundToInt(Radius / LandscapeScale.X);

    // Calculate bounds for modification
    int32 X1 = CenterX - RadiusInQuads;
    int32 Y1 = CenterY - RadiusInQuads;
    int32 X2 = CenterX + RadiusInQuads;
    int32 Y2 = CenterY + RadiusInQuads;

    // Create heightmap data interface
    FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);

    // Read current heights
    TArray<uint16> HeightData;
    int32 Width = X2 - X1 + 1;
    int32 Height = Y2 - Y1 + 1;
    HeightData.SetNumZeroed(Width * Height);

    LandscapeEdit.GetHeightDataFast(X1, Y1, X2, Y2, HeightData.GetData(), 0);

    // Modify heights with circular brush
    float MaxHeightChange = Strength * 100.0f * (bRaise ? 1.0f : -1.0f);

    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            float DistX = (X - RadiusInQuads);
            float DistY = (Y - RadiusInQuads);
            float Distance = FMath::Sqrt(DistX * DistX + DistY * DistY);

            if (Distance <= RadiusInQuads)
            {
                // Calculate falloff
                float NormalizedDist = Distance / RadiusInQuads;
                float FalloffFactor = 1.0f - FMath::Pow(NormalizedDist, 1.0f / FMath::Max(Falloff, 0.01f));

                int32 Index = Y * Width + X;
                int32 CurrentHeight = HeightData[Index];
                int32 HeightChange = FMath::RoundToInt(MaxHeightChange * FalloffFactor);
                int32 NewHeight = FMath::Clamp(CurrentHeight + HeightChange, 0, 65535);
                HeightData[Index] = (uint16)NewHeight;
            }
        }
    }

    // Write modified heights back
    LandscapeEdit.SetHeightData(X1, Y1, X2, Y2, HeightData.GetData(), 0, true);

    // Flush and update
    LandscapeEdit.Flush();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), TargetLandscape->GetName());

    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    Result->SetArrayField(TEXT("location"), LocationArray);

    Result->SetNumberField(TEXT("radius"), Radius);
    Result->SetNumberField(TEXT("strength"), Strength);
    Result->SetBoolField(TEXT("raised"), bRaise);
    Result->SetStringField(TEXT("message"), bRaise ? TEXT("Terrain raised successfully") : TEXT("Terrain lowered successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleSmoothLandscape(const TSharedPtr<FJsonObject>& Params)
{
    FVector Location(0, 0, 0);
    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'location' parameter"));
    }

    float Radius = 500.0f;
    if (Params->HasField(TEXT("radius")))
    {
        Radius = Params->GetNumberField(TEXT("radius"));
    }

    float Strength = 0.5f;
    if (Params->HasField(TEXT("strength")))
    {
        Strength = Params->GetNumberField(TEXT("strength"));
    }

    int32 Iterations = 1;
    if (Params->HasField(TEXT("iterations")))
    {
        Iterations = FMath::Clamp((int32)Params->GetNumberField(TEXT("iterations")), 1, 10);
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Find landscape at location
    ALandscapeProxy* TargetLandscape = nullptr;
    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            FBox Bounds = LandscapeProxy->GetComponentsBoundingBox();
            if (Location.X >= Bounds.Min.X && Location.X <= Bounds.Max.X &&
                Location.Y >= Bounds.Min.Y && Location.Y <= Bounds.Max.Y)
            {
                TargetLandscape = LandscapeProxy;
                break;
            }
        }
    }

    if (!TargetLandscape)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found at specified location"));
    }

    ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
    if (!LandscapeInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get landscape info"));
    }

    FVector LandscapeLocation = TargetLandscape->GetActorLocation();
    FVector LandscapeScale = TargetLandscape->GetActorScale3D();

    int32 CenterX = FMath::RoundToInt((Location.X - LandscapeLocation.X) / LandscapeScale.X);
    int32 CenterY = FMath::RoundToInt((Location.Y - LandscapeLocation.Y) / LandscapeScale.Y);
    int32 RadiusInQuads = FMath::RoundToInt(Radius / LandscapeScale.X);

    int32 X1 = CenterX - RadiusInQuads;
    int32 Y1 = CenterY - RadiusInQuads;
    int32 X2 = CenterX + RadiusInQuads;
    int32 Y2 = CenterY + RadiusInQuads;

    FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);

    int32 Width = X2 - X1 + 1;
    int32 Height = Y2 - Y1 + 1;
    TArray<uint16> HeightData;
    HeightData.SetNumZeroed(Width * Height);

    LandscapeEdit.GetHeightDataFast(X1, Y1, X2, Y2, HeightData.GetData(), 0);

    // Apply smoothing iterations
    TArray<uint16> SmoothedData = HeightData;

    for (int32 Iter = 0; Iter < Iterations; Iter++)
    {
        for (int32 Y = 1; Y < Height - 1; Y++)
        {
            for (int32 X = 1; X < Width - 1; X++)
            {
                float DistX = (X - RadiusInQuads);
                float DistY = (Y - RadiusInQuads);
                float Distance = FMath::Sqrt(DistX * DistX + DistY * DistY);

                if (Distance <= RadiusInQuads)
                {
                    // Get neighbors average
                    float Sum = 0;
                    Sum += HeightData[(Y - 1) * Width + (X - 1)];
                    Sum += HeightData[(Y - 1) * Width + X];
                    Sum += HeightData[(Y - 1) * Width + (X + 1)];
                    Sum += HeightData[Y * Width + (X - 1)];
                    Sum += HeightData[Y * Width + X];
                    Sum += HeightData[Y * Width + (X + 1)];
                    Sum += HeightData[(Y + 1) * Width + (X - 1)];
                    Sum += HeightData[(Y + 1) * Width + X];
                    Sum += HeightData[(Y + 1) * Width + (X + 1)];
                    float Average = Sum / 9.0f;

                    int32 Index = Y * Width + X;
                    float NormalizedDist = Distance / RadiusInQuads;
                    float BlendFactor = Strength * (1.0f - NormalizedDist);

                    float NewHeight = FMath::Lerp((float)HeightData[Index], Average, BlendFactor);
                    SmoothedData[Index] = (uint16)FMath::Clamp(FMath::RoundToInt(NewHeight), 0, 65535);
                }
            }
        }
        HeightData = SmoothedData;
    }

    LandscapeEdit.SetHeightData(X1, Y1, X2, Y2, SmoothedData.GetData(), 0, true);
    LandscapeEdit.Flush();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), TargetLandscape->GetName());
    Result->SetNumberField(TEXT("radius"), Radius);
    Result->SetNumberField(TEXT("strength"), Strength);
    Result->SetNumberField(TEXT("iterations"), Iterations);
    Result->SetStringField(TEXT("message"), TEXT("Terrain smoothed successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleFlattenLandscape(const TSharedPtr<FJsonObject>& Params)
{
    FVector Location(0, 0, 0);
    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'location' parameter"));
    }

    float Radius = 500.0f;
    if (Params->HasField(TEXT("radius")))
    {
        Radius = Params->GetNumberField(TEXT("radius"));
    }

    float Strength = 1.0f;
    if (Params->HasField(TEXT("strength")))
    {
        Strength = Params->GetNumberField(TEXT("strength"));
    }

    // Target height - if not specified, use height at center
    float TargetHeight = 0.0f;
    bool bUseLocationHeight = !Params->HasField(TEXT("target_height"));
    if (!bUseLocationHeight)
    {
        TargetHeight = Params->GetNumberField(TEXT("target_height"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    ALandscapeProxy* TargetLandscape = nullptr;
    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            FBox Bounds = LandscapeProxy->GetComponentsBoundingBox();
            if (Location.X >= Bounds.Min.X && Location.X <= Bounds.Max.X &&
                Location.Y >= Bounds.Min.Y && Location.Y <= Bounds.Max.Y)
            {
                TargetLandscape = LandscapeProxy;
                break;
            }
        }
    }

    if (!TargetLandscape)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found at specified location"));
    }

    ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
    if (!LandscapeInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get landscape info"));
    }

    FVector LandscapeLocation = TargetLandscape->GetActorLocation();
    FVector LandscapeScale = TargetLandscape->GetActorScale3D();

    int32 CenterX = FMath::RoundToInt((Location.X - LandscapeLocation.X) / LandscapeScale.X);
    int32 CenterY = FMath::RoundToInt((Location.Y - LandscapeLocation.Y) / LandscapeScale.Y);
    int32 RadiusInQuads = FMath::RoundToInt(Radius / LandscapeScale.X);

    int32 X1 = CenterX - RadiusInQuads;
    int32 Y1 = CenterY - RadiusInQuads;
    int32 X2 = CenterX + RadiusInQuads;
    int32 Y2 = CenterY + RadiusInQuads;

    FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);

    int32 Width = X2 - X1 + 1;
    int32 Height = Y2 - Y1 + 1;
    TArray<uint16> HeightData;
    HeightData.SetNumZeroed(Width * Height);

    LandscapeEdit.GetHeightDataFast(X1, Y1, X2, Y2, HeightData.GetData(), 0);

    // Get target height from center if not specified
    uint16 TargetHeightValue;
    if (bUseLocationHeight)
    {
        TargetHeightValue = HeightData[RadiusInQuads * Width + RadiusInQuads];
    }
    else
    {
        // Convert world height to landscape height value
        float HeightRange = 512.0f * LandscapeScale.Z * 2.0f; // Approximate
        TargetHeightValue = (uint16)FMath::Clamp(
            FMath::RoundToInt(((TargetHeight - LandscapeLocation.Z) / HeightRange + 0.5f) * 65535.0f),
            0, 65535);
    }

    // Flatten to target height
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            float DistX = (X - RadiusInQuads);
            float DistY = (Y - RadiusInQuads);
            float Distance = FMath::Sqrt(DistX * DistX + DistY * DistY);

            if (Distance <= RadiusInQuads)
            {
                int32 Index = Y * Width + X;
                float NormalizedDist = Distance / RadiusInQuads;
                float BlendFactor = Strength * (1.0f - NormalizedDist);

                float NewHeight = FMath::Lerp((float)HeightData[Index], (float)TargetHeightValue, BlendFactor);
                HeightData[Index] = (uint16)FMath::Clamp(FMath::RoundToInt(NewHeight), 0, 65535);
            }
        }
    }

    LandscapeEdit.SetHeightData(X1, Y1, X2, Y2, HeightData.GetData(), 0, true);
    LandscapeEdit.Flush();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), TargetLandscape->GetName());
    Result->SetNumberField(TEXT("radius"), Radius);
    Result->SetNumberField(TEXT("strength"), Strength);
    Result->SetStringField(TEXT("message"), TEXT("Terrain flattened successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandlePaintLandscapeLayer(const TSharedPtr<FJsonObject>& Params)
{
    FVector Location(0, 0, 0);
    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'location' parameter"));
    }

    FString LayerName;
    if (!Params->TryGetStringField(TEXT("layer_name"), LayerName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'layer_name' parameter"));
    }

    float Radius = 500.0f;
    if (Params->HasField(TEXT("radius")))
    {
        Radius = Params->GetNumberField(TEXT("radius"));
    }

    float Strength = 1.0f;
    if (Params->HasField(TEXT("strength")))
    {
        Strength = FMath::Clamp(Params->GetNumberField(TEXT("strength")), 0.0f, 1.0f);
    }

    float Falloff = 0.5f;
    if (Params->HasField(TEXT("falloff")))
    {
        Falloff = Params->GetNumberField(TEXT("falloff"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    ALandscapeProxy* TargetLandscape = nullptr;
    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            FBox Bounds = LandscapeProxy->GetComponentsBoundingBox();
            if (Location.X >= Bounds.Min.X && Location.X <= Bounds.Max.X &&
                Location.Y >= Bounds.Min.Y && Location.Y <= Bounds.Max.Y)
            {
                TargetLandscape = LandscapeProxy;
                break;
            }
        }
    }

    if (!TargetLandscape)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found at specified location"));
    }

    ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
    if (!LandscapeInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get landscape info"));
    }

    // Find the layer info
    ULandscapeLayerInfoObject* TargetLayerInfo = nullptr;
    for (const FLandscapeInfoLayerSettings& LayerSettings : LandscapeInfo->Layers)
    {
        if (LayerSettings.LayerInfoObj && LayerSettings.LayerInfoObj->GetLayerName().ToString() == LayerName)
        {
            TargetLayerInfo = LayerSettings.LayerInfoObj;
            break;
        }
    }

    if (!TargetLayerInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Layer '%s' not found"), *LayerName));
    }

    FVector LandscapeLocation = TargetLandscape->GetActorLocation();
    FVector LandscapeScale = TargetLandscape->GetActorScale3D();

    int32 CenterX = FMath::RoundToInt((Location.X - LandscapeLocation.X) / LandscapeScale.X);
    int32 CenterY = FMath::RoundToInt((Location.Y - LandscapeLocation.Y) / LandscapeScale.Y);
    int32 RadiusInQuads = FMath::RoundToInt(Radius / LandscapeScale.X);

    int32 X1 = CenterX - RadiusInQuads;
    int32 Y1 = CenterY - RadiusInQuads;
    int32 X2 = CenterX + RadiusInQuads;
    int32 Y2 = CenterY + RadiusInQuads;

    FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);

    int32 Width = X2 - X1 + 1;
    int32 Height = Y2 - Y1 + 1;

    // Get current weight data for this layer
    TArray<uint8> WeightData;
    WeightData.SetNumZeroed(Width * Height);

    LandscapeEdit.GetWeightDataFast(TargetLayerInfo, X1, Y1, X2, Y2, WeightData.GetData(), 0);

    // Paint the layer
    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            float DistX = (X - RadiusInQuads);
            float DistY = (Y - RadiusInQuads);
            float Distance = FMath::Sqrt(DistX * DistX + DistY * DistY);

            if (Distance <= RadiusInQuads)
            {
                int32 Index = Y * Width + X;
                float NormalizedDist = Distance / RadiusInQuads;
                float FalloffFactor = 1.0f - FMath::Pow(NormalizedDist, 1.0f / FMath::Max(Falloff, 0.01f));

                float PaintStrength = Strength * FalloffFactor;
                uint8 NewWeight = (uint8)FMath::Clamp(
                    FMath::RoundToInt(FMath::Lerp((float)WeightData[Index], 255.0f, PaintStrength)),
                    0, 255);
                WeightData[Index] = NewWeight;
            }
        }
    }

    LandscapeEdit.SetAlphaData(TargetLayerInfo, X1, Y1, X2, Y2, WeightData.GetData(), 0);
    LandscapeEdit.Flush();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), TargetLandscape->GetName());
    Result->SetStringField(TEXT("layer"), LayerName);
    Result->SetNumberField(TEXT("radius"), Radius);
    Result->SetNumberField(TEXT("strength"), Strength);
    Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Layer '%s' painted successfully"), *LayerName));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleGetLandscapeLayers(const TSharedPtr<FJsonObject>& Params)
{
    FString LandscapeName;
    Params->TryGetStringField(TEXT("landscape_name"), LandscapeName);

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    ALandscapeProxy* TargetLandscape = nullptr;
    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            if (LandscapeName.IsEmpty() || LandscapeProxy->GetName() == LandscapeName)
            {
                TargetLandscape = LandscapeProxy;
                break;
            }
        }
    }

    if (!TargetLandscape)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found"));
    }

    TArray<TSharedPtr<FJsonValue>> LayersArray;
    ULandscapeInfo* LandscapeInfoRef = TargetLandscape->GetLandscapeInfo();
    if (LandscapeInfoRef)
    {
        for (const FLandscapeInfoLayerSettings& LayerSettings : LandscapeInfoRef->Layers)
        {
            if (LayerSettings.LayerInfoObj)
            {
                TSharedPtr<FJsonObject> LayerObj = MakeShared<FJsonObject>();
                LayerObj->SetStringField(TEXT("name"), LayerSettings.LayerInfoObj->GetLayerName().ToString());
                LayerObj->SetStringField(TEXT("path"), LayerSettings.LayerInfoObj->GetPathName());
                LayersArray.Add(MakeShared<FJsonValueObject>(LayerObj));
            }
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), TargetLandscape->GetName());
    Result->SetArrayField(TEXT("layers"), LayersArray);
    Result->SetNumberField(TEXT("count"), LayersArray.Num());

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleSetLandscapeMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString LandscapeName;
    Params->TryGetStringField(TEXT("landscape_name"), LandscapeName);

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Load material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    // Set material on ALL landscape proxies (main + streaming) so they all match
    int32 UpdatedCount = 0;
    FString MainLandscapeName;
    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* Proxy = Cast<ALandscapeProxy>(Actor);
        if (!Proxy) continue;

        // If user specified a name, only match that specific main landscape
        if (!LandscapeName.IsEmpty())
        {
            ALandscape* AsMainLandscape = Cast<ALandscape>(Proxy);
            if (AsMainLandscape && AsMainLandscape->GetName() != LandscapeName) continue;
            // For non-main proxies, check if their parent matches
            if (!AsMainLandscape)
            {
                ALandscape* Parent = Proxy->GetLandscapeActor();
                if (Parent && Parent->GetName() != LandscapeName) continue;
            }
        }

        Proxy->LandscapeMaterial = Material;
        Proxy->MarkPackageDirty();

        // Update all components on this proxy
        TArray<ULandscapeComponent*> LandscapeComponents;
        Proxy->GetComponents<ULandscapeComponent>(LandscapeComponents);
        for (ULandscapeComponent* Comp : LandscapeComponents)
        {
            Comp->MarkRenderStateDirty();
        }

        if (Cast<ALandscape>(Proxy))
        {
            MainLandscapeName = Proxy->GetName();
        }
        UpdatedCount++;
    }

    if (UpdatedCount == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found"));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), MainLandscapeName);
    Result->SetStringField(TEXT("material"), MaterialPath);
    Result->SetNumberField(TEXT("proxies_updated"), UpdatedCount);
    Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Landscape material set on %d proxies"), UpdatedCount));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleCreateLandscapeLayer(const TSharedPtr<FJsonObject>& Params)
{
    FString LayerName;
    if (!Params->TryGetStringField(TEXT("layer_name"), LayerName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'layer_name' parameter"));
    }

    FString SavePath = TEXT("/Game/Landscape/Layers/");
    Params->TryGetStringField(TEXT("save_path"), SavePath);

    if (!SavePath.EndsWith(TEXT("/")))
    {
        SavePath += TEXT("/");
    }

    bool bNoWeightBlend = false;
    Params->TryGetBoolField(TEXT("no_weight_blend"), bNoWeightBlend);

    // Create the package - use LI_ prefix for the asset name
    FString AssetName = TEXT("LI_") + LayerName;
    FString PackagePath = SavePath + AssetName;
    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package for layer info"));
    }

    // Create the layer info object - object name must match asset name for proper loading
    ULandscapeLayerInfoObject* LayerInfo = NewObject<ULandscapeLayerInfoObject>(
        Package,
        FName(*AssetName),
        RF_Public | RF_Standalone
    );

    if (!LayerInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create layer info object"));
    }

    // Set layer properties - second param is bInModify (whether to mark package dirty)
    LayerInfo->SetLayerName(FName(*LayerName), true);

    // Initialize required properties to prevent crashes in Landscape Editor
    LayerInfo->Hardness = 0.5f;

    // Mark package dirty and notify asset registry
    Package->MarkPackageDirty();
    IAssetRegistry::Get()->AssetCreated(LayerInfo);

    // Save the asset to disk using UPackage::SavePackage
    FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());

    // Ensure the directory exists
    FString PackageDirectory = FPaths::GetPath(PackageFilename);
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*PackageDirectory))
    {
        PlatformFile.CreateDirectoryTree(*PackageDirectory);
    }

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, LayerInfo, *PackageFilename, SaveArgs);
    if (!bSaved)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to save layer info asset: %s"), *PackageFilename));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("layer_name"), LayerName);
    Result->SetStringField(TEXT("path"), PackagePath);
    Result->SetStringField(TEXT("message"), TEXT("Landscape layer created and saved successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPLandscapeCommands::HandleAddLayerToLandscape(const TSharedPtr<FJsonObject>& Params)
{
    FString LayerPath;
    if (!Params->TryGetStringField(TEXT("layer_path"), LayerPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'layer_path' parameter"));
    }

    FString LandscapeName;
    Params->TryGetStringField(TEXT("landscape_name"), LandscapeName);

    // Try multiple methods to load the layer info
    ULandscapeLayerInfoObject* LayerInfo = nullptr;

    // Method 1: Try UEditorAssetLibrary
    LayerInfo = Cast<ULandscapeLayerInfoObject>(UEditorAssetLibrary::LoadAsset(LayerPath));

    // Method 2: Try LoadPackage + FindObject if the first method failed
    if (!LayerInfo)
    {
        FString PackageName = LayerPath;
        UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
        if (Package)
        {
            Package->FullyLoad();
            // Get the asset name from the path (last part after /)
            FString AssetName = FPackageName::GetShortName(LayerPath);
            LayerInfo = FindObject<ULandscapeLayerInfoObject>(Package, *AssetName);
        }
    }

    // Method 3: Try StaticLoadObject with explicit class
    if (!LayerInfo)
    {
        FString ObjectPath = LayerPath + TEXT(".") + FPackageName::GetShortName(LayerPath);
        LayerInfo = Cast<ULandscapeLayerInfoObject>(StaticLoadObject(ULandscapeLayerInfoObject::StaticClass(), nullptr, *ObjectPath));
    }

    if (!LayerInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Layer info not found: %s (tried all loading methods)"), *LayerPath));
    }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscapeProxy::StaticClass(), LandscapeActors);

    ALandscapeProxy* TargetLandscape = nullptr;
    for (AActor* Actor : LandscapeActors)
    {
        ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
        if (LandscapeProxy)
        {
            if (LandscapeName.IsEmpty() || LandscapeProxy->GetName() == LandscapeName)
            {
                TargetLandscape = LandscapeProxy;
                break;
            }
        }
    }

    if (!TargetLandscape)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No landscape found"));
    }

    ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
    if (!LandscapeInfo)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get landscape info"));
    }

    // Add the layer to the landscape
    int32 LayerIndex = LandscapeInfo->Layers.Num();

    FLandscapeInfoLayerSettings NewLayerSettings;
    NewLayerSettings.LayerInfoObj = LayerInfo;
    NewLayerSettings.LayerName = LayerInfo->GetLayerName();

    LandscapeInfo->Layers.Add(NewLayerSettings);

    // Mark dirty
    TargetLandscape->MarkPackageDirty();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("landscape"), TargetLandscape->GetName());
    Result->SetStringField(TEXT("layer"), LayerInfo->GetLayerName().ToString());
    Result->SetNumberField(TEXT("layer_index"), LayerIndex);
    Result->SetStringField(TEXT("message"), TEXT("Layer added to landscape successfully"));

    return Result;
}
