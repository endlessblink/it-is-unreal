#include "Commands/EpicUnrealMCPEditorCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Widgets/SViewport.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/DecalComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/DecalActor.h"
#include "Atmosphere/AtmosphericFogComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EditorAssetLibrary.h"
#include "Commands/EpicUnrealMCPBlueprintCommands.h"
#include "UObject/UnrealType.h"

// Material and Texture includes
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionCosine.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionDistance.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "MaterialEditingLibrary.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"

// Screenshot / Image encoding
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"

// Asset Registry
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetRegistryModule.h"

// FBX/Mesh Import
#include "AssetImportTask.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Engine/StaticMesh.h"

// Skeletal Mesh / Animation Import
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"

// HISM for foliage scatter
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

// Audio import
#include "Sound/SoundWave.h"
#include "Sound/AmbientSound.h"
#include "Components/AudioComponent.h"
#include "Factories/SoundFactory.h"

// Landscape filter for foliage scatter line traces
#include "LandscapeProxy.h"

// AppendVector for UV distortion
#include "Materials/MaterialExpressionAppendVector.h"

FEpicUnrealMCPEditorCommands::FEpicUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor"))
    {
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Actor property manipulation
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    // Material commands
    else if (CommandType == TEXT("create_material"))
    {
        return HandleCreateMaterial(Params);
    }
    else if (CommandType == TEXT("create_material_instance"))
    {
        return HandleCreateMaterialInstance(Params);
    }
    else if (CommandType == TEXT("set_material_instance_parameter"))
    {
        return HandleSetMaterialInstanceParameter(Params);
    }
    // Texture commands
    else if (CommandType == TEXT("import_texture"))
    {
        return HandleImportTexture(Params);
    }
    else if (CommandType == TEXT("set_texture_properties"))
    {
        return HandleSetTextureProperties(Params);
    }
    else if (CommandType == TEXT("create_pbr_material"))
    {
        return HandleCreatePBRMaterial(Params);
    }
    else if (CommandType == TEXT("create_landscape_material"))
    {
        return HandleCreateLandscapeMaterial(Params);
    }
    // Asset import and management commands
    else if (CommandType == TEXT("import_mesh"))
    {
        return HandleImportMesh(Params);
    }
    else if (CommandType == TEXT("import_skeletal_mesh"))
    {
        return HandleImportSkeletalMesh(Params);
    }
    else if (CommandType == TEXT("import_animation"))
    {
        return HandleImportAnimation(Params);
    }
    else if (CommandType == TEXT("list_assets"))
    {
        return HandleListAssets(Params);
    }
    else if (CommandType == TEXT("does_asset_exist"))
    {
        return HandleDoesAssetExist(Params);
    }
    else if (CommandType == TEXT("get_asset_info"))
    {
        return HandleGetAssetInfo(Params);
    }
    // World query commands
    else if (CommandType == TEXT("get_height_at_location"))
    {
        return HandleGetHeightAtLocation(Params);
    }
    else if (CommandType == TEXT("snap_actor_to_ground"))
    {
        return HandleSnapActorToGround(Params);
    }
    else if (CommandType == TEXT("scatter_meshes_on_landscape"))
    {
        return HandleScatterMeshesOnLandscape(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    else if (CommandType == TEXT("get_material_info"))
    {
        return HandleGetMaterialInfo(Params);
    }
    else if (CommandType == TEXT("focus_viewport_on_actor"))
    {
        return HandleFocusViewportOnActor(Params);
    }
    else if (CommandType == TEXT("get_texture_info"))
    {
        return HandleGetTextureInfo(Params);
    }
    else if (CommandType == TEXT("delete_actors_by_pattern"))
    {
        return HandleDeleteActorsByPattern(Params);
    }
    // Asset deletion
    else if (CommandType == TEXT("delete_asset"))
    {
        return HandleDeleteAsset(Params);
    }
    // Mesh asset properties
    else if (CommandType == TEXT("set_nanite_enabled"))
    {
        return HandleSetNaniteEnabled(Params);
    }
    // HISM foliage scatter
    else if (CommandType == TEXT("scatter_foliage"))
    {
        return HandleScatterFoliage(Params);
    }
    // Audio import
    else if (CommandType == TEXT("import_sound"))
    {
        return HandleImportSound(Params);
    }
    // Animation notify
    else if (CommandType == TEXT("add_anim_notify"))
    {
        return HandleAddAnimNotify(Params);
    }
    // Editor log reading
    else if (CommandType == TEXT("get_editor_log"))
    {
        return HandleGetEditorLog(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FEpicUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        AStaticMeshActor* NewMeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
        if (NewMeshActor)
        {
            // Check for an optional static_mesh parameter to assign a mesh
            FString MeshPath;
            if (Params->TryGetStringField(TEXT("static_mesh"), MeshPath))
            {
                UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
                if (Mesh)
                {
                    NewMeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Could not find static mesh at path: %s"), *MeshPath);
                }
            }
        }
        NewActor = NewMeshActor;
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CineCameraActor"))
    {
        NewActor = World->SpawnActor<ACineCameraActor>(ACineCameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("ExponentialHeightFog"))
    {
        NewActor = World->SpawnActor<AExponentialHeightFog>(AExponentialHeightFog::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SkyLight"))
    {
        NewActor = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("PostProcessVolume"))
    {
        APostProcessVolume* PPVolume = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), Location, Rotation, SpawnParams);
        if (PPVolume)
        {
            // Check for optional bUnbound parameter
            bool bUnbound = true;
            if (Params->TryGetBoolField(TEXT("unbound"), bUnbound))
            {
                PPVolume->bUnbound = bUnbound;
            }
            else
            {
                // Default to unbound so it affects the whole level
                PPVolume->bUnbound = true;
            }
        }
        NewActor = PPVolume;
    }
    else if (ActorType == TEXT("AmbientSound"))
    {
        AAmbientSound* SoundActor = World->SpawnActor<AAmbientSound>(AAmbientSound::StaticClass(), Location, Rotation, SpawnParams);
        if (SoundActor)
        {
            UAudioComponent* AudioComp = SoundActor->GetAudioComponent();
            if (AudioComp)
            {
                // Set sound asset
                FString SoundPath;
                if (Params->TryGetStringField(TEXT("sound_asset"), SoundPath))
                {
                    USoundBase* Sound = Cast<USoundBase>(UEditorAssetLibrary::LoadAsset(SoundPath));
                    if (Sound)
                    {
                        AudioComp->SetSound(Sound);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Could not find sound asset at path: %s"), *SoundPath);
                    }
                }

                // Volume multiplier
                double VolumeMultiplier = 1.0;
                if (Params->TryGetNumberField(TEXT("volume_multiplier"), VolumeMultiplier))
                {
                    AudioComp->VolumeMultiplier = static_cast<float>(VolumeMultiplier);
                }

                // Pitch multiplier
                double PitchMultiplier = 1.0;
                if (Params->TryGetNumberField(TEXT("pitch_multiplier"), PitchMultiplier))
                {
                    AudioComp->PitchMultiplier = static_cast<float>(PitchMultiplier);
                }

                // Auto activate
                bool bAutoActivate = true;
                if (Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate))
                {
                    AudioComp->SetAutoActivate(bAutoActivate);
                }

                // UI sound (non-spatialized, for music)
                bool bUISound = false;
                if (Params->TryGetBoolField(TEXT("is_ui_sound"), bUISound) && bUISound)
                {
                    AudioComp->bIsUISound = true;
                    AudioComp->bAllowSpatialization = false;
                }

                // Attenuation override for max distance
                double AttenuationMaxDist = 0.0;
                if (Params->TryGetNumberField(TEXT("attenuation_max_distance"), AttenuationMaxDist))
                {
                    AudioComp->bOverrideAttenuation = true;
                    AudioComp->AttenuationOverrides.FalloffDistance = static_cast<float>(AttenuationMaxDist);
                }
            }
        }
        NewActor = SoundActor;
    }
    else if (ActorType == TEXT("DecalActor"))
    {
        ADecalActor* DecalActorObj = World->SpawnActor<ADecalActor>(ADecalActor::StaticClass(), Location, Rotation, SpawnParams);
        if (DecalActorObj)
        {
            // Check for optional decal material
            FString MaterialPath;
            if (Params->TryGetStringField(TEXT("decal_material"), MaterialPath))
            {
                UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
                if (Material)
                {
                    DecalActorObj->SetDecalMaterial(Material);
                }
            }
            // Check for optional decal size
            if (Params->HasField(TEXT("decal_size")))
            {
                FVector DecalSize = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("decal_size"));
                DecalActorObj->GetDecal()->DecalSize = DecalSize;
            }
        }
        NewActor = DecalActorObj;
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (IsValid(Actor) && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FEpicUnrealMCPCommonUtils::ActorToJsonObject(Actor);

            // Use EditorActorSubsystem for safe editor deletion.
            // It handles OFPA packages, scene outliner, and editor notifications.
            UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
            if (EditorActorSubsystem)
            {
                EditorActorSubsystem->DestroyActor(Actor);
            }
            else
            {
                World->DestroyActor(Actor);
            }

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FEpicUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // This function will now correctly call the implementation in BlueprintCommands
    FEpicUnrealMCPBlueprintCommands BlueprintCommands;
    return BlueprintCommands.HandleCommand(TEXT("spawn_blueprint_actor"), Params);
}

// Helper function to get FLinearColor from JSON
static FLinearColor GetLinearColorFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
    FLinearColor Result(1.0f, 1.0f, 1.0f, 1.0f);

    if (JsonObject->HasField(TEXT("R")))
    {
        Result.R = JsonObject->GetNumberField(TEXT("R"));
    }
    if (JsonObject->HasField(TEXT("G")))
    {
        Result.G = JsonObject->GetNumberField(TEXT("G"));
    }
    if (JsonObject->HasField(TEXT("B")))
    {
        Result.B = JsonObject->GetNumberField(TEXT("B"));
    }
    if (JsonObject->HasField(TEXT("A")))
    {
        Result.A = JsonObject->GetNumberField(TEXT("A"));
    }

    return Result;
}

// Helper function to get FVector4 from JSON
static FVector4 GetVector4FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
    FVector4 Result(1.0f, 1.0f, 1.0f, 1.0f);

    if (JsonObject->HasField(TEXT("X")))
    {
        Result.X = JsonObject->GetNumberField(TEXT("X"));
    }
    if (JsonObject->HasField(TEXT("Y")))
    {
        Result.Y = JsonObject->GetNumberField(TEXT("Y"));
    }
    if (JsonObject->HasField(TEXT("Z")))
    {
        Result.Z = JsonObject->GetNumberField(TEXT("Z"));
    }
    if (JsonObject->HasField(TEXT("W")))
    {
        Result.W = JsonObject->GetNumberField(TEXT("W"));
    }

    return Result;
}

// Helper function to set a property value using Unreal's reflection system
static bool SetPropertyValue(UObject* TargetObject, FProperty* Property, const TSharedPtr<FJsonValue>& Value, FString& OutError, void* OverrideAddress = nullptr)
{
    if (!Property || (!TargetObject && !OverrideAddress))
    {
        OutError = TEXT("Invalid target object or property");
        return false;
    }

    void* PropertyAddr = OverrideAddress ? OverrideAddress : Property->ContainerPtrToValuePtr<void>(TargetObject);

    // Handle bool property
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
        BoolProp->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    // Handle float property
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
        FloatProp->SetPropertyValue(PropertyAddr, static_cast<float>(Value->AsNumber()));
        return true;
    }
    // Handle double property
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
        DoubleProp->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    // Handle int property
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
        IntProp->SetPropertyValue(PropertyAddr, static_cast<int32>(Value->AsNumber()));
        return true;
    }
    // Handle string property
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        StrProp->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    // Handle struct properties (FLinearColor, FVector, FVector4, etc.)
    else if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        UScriptStruct* Struct = StructProp->Struct;
        FString StructName = Struct->GetName();

        // Handle FLinearColor
        if (StructName == TEXT("LinearColor"))
        {
            const TSharedPtr<FJsonObject>* JsonObj;
            if (Value->TryGetObject(JsonObj))
            {
                FLinearColor* ColorPtr = static_cast<FLinearColor*>(PropertyAddr);
                *ColorPtr = GetLinearColorFromJson(*JsonObj);
                return true;
            }
        }
        // Handle FColor
        else if (StructName == TEXT("Color"))
        {
            const TSharedPtr<FJsonObject>* JsonObj;
            if (Value->TryGetObject(JsonObj))
            {
                FLinearColor LinearColor = GetLinearColorFromJson(*JsonObj);
                FColor* ColorPtr = static_cast<FColor*>(PropertyAddr);
                *ColorPtr = LinearColor.ToFColor(true);
                return true;
            }
        }
        // Handle FVector
        else if (StructName == TEXT("Vector"))
        {
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (Value->TryGetArray(JsonArray) && JsonArray->Num() >= 3)
            {
                FVector* VectorPtr = static_cast<FVector*>(PropertyAddr);
                VectorPtr->X = (*JsonArray)[0]->AsNumber();
                VectorPtr->Y = (*JsonArray)[1]->AsNumber();
                VectorPtr->Z = (*JsonArray)[2]->AsNumber();
                return true;
            }
        }
        // Handle FVector4
        else if (StructName == TEXT("Vector4"))
        {
            const TSharedPtr<FJsonObject>* JsonObj;
            if (Value->TryGetObject(JsonObj))
            {
                FVector4* Vector4Ptr = static_cast<FVector4*>(PropertyAddr);
                *Vector4Ptr = GetVector4FromJson(*JsonObj);
                return true;
            }
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (Value->TryGetArray(JsonArray) && JsonArray->Num() >= 4)
            {
                FVector4* Vector4Ptr = static_cast<FVector4*>(PropertyAddr);
                Vector4Ptr->X = (*JsonArray)[0]->AsNumber();
                Vector4Ptr->Y = (*JsonArray)[1]->AsNumber();
                Vector4Ptr->Z = (*JsonArray)[2]->AsNumber();
                Vector4Ptr->W = (*JsonArray)[3]->AsNumber();
                return true;
            }
        }
        // Handle FRotator
        else if (StructName == TEXT("Rotator"))
        {
            const TArray<TSharedPtr<FJsonValue>>* JsonArray;
            if (Value->TryGetArray(JsonArray) && JsonArray->Num() >= 3)
            {
                FRotator* RotatorPtr = static_cast<FRotator*>(PropertyAddr);
                RotatorPtr->Pitch = (*JsonArray)[0]->AsNumber();
                RotatorPtr->Yaw = (*JsonArray)[1]->AsNumber();
                RotatorPtr->Roll = (*JsonArray)[2]->AsNumber();
                return true;
            }
        }

        OutError = FString::Printf(TEXT("Unsupported struct type: %s"), *StructName);
        return false;
    }
    // Handle byte/enum-as-byte properties (e.g., AnimationMode, CollisionEnabled)
    else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
    {
        UEnum* EnumDef = ByteProp->GetIntPropertyEnum();

        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);

                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"),
                      *Property->GetName(), ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();

                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"),
                          *Property->GetName(), *EnumValueName, ByteValue);
                    return true;
                }

                // Handle qualified enum names (e.g., "EAnimationMode::AnimationSingleNode")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }

                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }

                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"),
                          *Property->GetName(), *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"),
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }

                    OutError = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property (no enum)
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    // Handle native enum properties (C++ enum class)
    else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
    {
        UEnum* EnumDef = EnumProp->GetEnum();
        FNumericProperty* UnderlyingNumericProp = EnumProp->GetUnderlyingProperty();

        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);

                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"),
                      *Property->GetName(), EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();

                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"),
                          *Property->GetName(), *EnumValueName, EnumValue);
                    return true;
                }

                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }

                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }

                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);

                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"),
                          *Property->GetName(), *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"),
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }

                    OutError = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    // Handle class properties BEFORE object properties (FClassProperty inherits from FObjectProperty)
    else if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FString ClassPath = Value->AsString();
            UClass* LoadedClass = nullptr;

            // First try direct class loading (for C++ classes and _C suffix paths)
            LoadedClass = StaticLoadClass(ClassProp->MetaClass, nullptr, *ClassPath);

            // If direct loading failed, try loading as a Blueprint asset and getting its GeneratedClass
            if (!LoadedClass)
            {
                UObject* LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *ClassPath);
                if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedObject))
                {
                    LoadedClass = Blueprint->GeneratedClass;
                    UE_LOG(LogTemp, Display, TEXT("Loaded Blueprint %s, GeneratedClass: %s"),
                          *ClassPath, LoadedClass ? *LoadedClass->GetName() : TEXT("null"));
                }
                // Also try with _C suffix appended
                if (!LoadedClass && !ClassPath.EndsWith(TEXT("_C")))
                {
                    FString ClassPathWithSuffix = ClassPath;
                    // Extract package and asset name for _C path: /Game/Path/Asset -> /Game/Path/Asset.Asset_C
                    FString PackagePath, AssetName;
                    if (ClassPath.Split(TEXT("/"), &PackagePath, &AssetName, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
                    {
                        FString FullClassPath = FString::Printf(TEXT("%s/%s.%s_C"), *PackagePath, *AssetName, *AssetName);
                        LoadedClass = StaticLoadClass(ClassProp->MetaClass, nullptr, *FullClassPath);
                    }
                }
            }

            if (LoadedClass)
            {
                ClassProp->SetObjectPropertyValue(PropertyAddr, LoadedClass);
                UE_LOG(LogTemp, Display, TEXT("Setting class property %s to: %s (resolved: %s)"),
                      *Property->GetName(), *ClassPath, *LoadedClass->GetName());
                return true;
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to load class at path: %s (expected meta class: %s). Try: /Game/Path/AssetName or /Game/Path/AssetName.AssetName_C"),
                    *ClassPath, *ClassProp->MetaClass->GetName());
                return false;
            }
        }
        else if (Value->Type == EJson::Null)
        {
            ClassProp->SetObjectPropertyValue(PropertyAddr, nullptr);
            UE_LOG(LogTemp, Display, TEXT("Clearing class property %s"), *Property->GetName());
            return true;
        }
    }
    // Handle object properties (asset references: UTexture, UMaterial, UAnimationAsset, etc.)
    // NOTE: Must come AFTER FClassProperty check since FClassProperty inherits from FObjectProperty
    else if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FString AssetPath = Value->AsString();
            UObject* LoadedObject = StaticLoadObject(ObjProp->PropertyClass, nullptr, *AssetPath);
            if (LoadedObject)
            {
                ObjProp->SetObjectPropertyValue(PropertyAddr, LoadedObject);
                UE_LOG(LogTemp, Display, TEXT("Setting object property %s to: %s (class: %s)"),
                      *Property->GetName(), *AssetPath, *LoadedObject->GetClass()->GetName());
                return true;
            }
            else
            {
                OutError = FString::Printf(TEXT("Failed to load object at path: %s (expected class: %s)"),
                    *AssetPath, *ObjProp->PropertyClass->GetName());
                return false;
            }
        }
        else if (Value->Type == EJson::Null)
        {
            ObjProp->SetObjectPropertyValue(PropertyAddr, nullptr);
            UE_LOG(LogTemp, Display, TEXT("Clearing object property %s"), *Property->GetName());
            return true;
        }
    }
    // Handle soft object properties (TSoftObjectPtr<>)
    else if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FString AssetPath = Value->AsString();
            FSoftObjectPath SoftPath{AssetPath};
            FSoftObjectPtr SoftRef{SoftPath};
            SoftObjProp->SetPropertyValue(PropertyAddr, SoftRef);
            UE_LOG(LogTemp, Display, TEXT("Setting soft object property %s to: %s"),
                  *Property->GetName(), *AssetPath);
            return true;
        }
        else if (Value->Type == EJson::Null)
        {
            FSoftObjectPtr EmptyRef;
            SoftObjProp->SetPropertyValue(PropertyAddr, EmptyRef);
            return true;
        }
    }
    // Handle soft class properties (TSoftClassPtr<>)
    else if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FString ClassPath = Value->AsString();
            FSoftObjectPath SoftPath{ClassPath};
            FSoftObjectPtr SoftRef{SoftPath};
            SoftClassProp->SetPropertyValue(PropertyAddr, SoftRef);
            UE_LOG(LogTemp, Display, TEXT("Setting soft class property %s to: %s"),
                  *Property->GetName(), *ClassPath);
            return true;
        }
        else if (Value->Type == EJson::Null)
        {
            FSoftObjectPtr EmptyRef;
            SoftClassProp->SetPropertyValue(PropertyAddr, EmptyRef);
            return true;
        }
    }
    // Handle name properties (FName)
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FName NameValue = FName(*Value->AsString());
            NameProp->SetPropertyValue(PropertyAddr, NameValue);
            UE_LOG(LogTemp, Display, TEXT("Setting name property %s to: %s"),
                  *Property->GetName(), *Value->AsString());
            return true;
        }
    }
    // Handle text properties (FText)
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
        if (Value->Type == EJson::String)
        {
            FText TextValue = FText::FromString(Value->AsString());
            TextProp->SetPropertyValue(PropertyAddr, TextValue);
            UE_LOG(LogTemp, Display, TEXT("Setting text property %s to: %s"),
                  *Property->GetName(), *Value->AsString());
            return true;
        }
    }

    OutError = FString::Printf(TEXT("Unsupported property type: %s"), *Property->GetClass()->GetName());
    return false;
}

// Helper function to find component on an actor
static UActorComponent* FindComponentOnActor(AActor* Actor, const FString& ComponentName)
{
    if (!Actor)
    {
        return nullptr;
    }

    // If component name is provided, find it by name
    if (!ComponentName.IsEmpty())
    {
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);

        for (UActorComponent* Component : Components)
        {
            if (Component && Component->GetName() == ComponentName)
            {
                return Component;
            }
        }
        return nullptr;
    }

    // If no component name provided, try to find a suitable default component based on actor type

    // For light actors, return the light component
    if (ALight* LightActor = Cast<ALight>(Actor))
    {
        return LightActor->GetLightComponent();
    }

    // For exponential height fog
    if (AExponentialHeightFog* FogActor = Cast<AExponentialHeightFog>(Actor))
    {
        return FogActor->GetComponent();
    }

    // For sky light
    if (ASkyLight* SkyLightActor = Cast<ASkyLight>(Actor))
    {
        return SkyLightActor->GetLightComponent();
    }

    // For post process volume, return the root component which has the settings
    if (APostProcessVolume* PPVolume = Cast<APostProcessVolume>(Actor))
    {
        return PPVolume->GetRootComponent();
    }

    // Return root component as fallback
    return Actor->GetRootComponent();
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get optional component name
    FString ComponentName;
    Params->TryGetStringField(TEXT("component_name"), ComponentName);

    // Get property value (required)
    if (!Params->HasField(TEXT("property_value")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    TSharedPtr<FJsonValue> PropertyValue = Params->TryGetField(TEXT("property_value"));

    // Find the actor
    AActor* TargetActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Handle nested property paths (e.g., "Settings.ColorSaturation")
    TArray<FString> PropertyPath;
    PropertyName.ParseIntoArray(PropertyPath, TEXT("."), true);

    UObject* CurrentObject = nullptr;

    // Special handling for PostProcessVolume settings
    if (APostProcessVolume* PPVolume = Cast<APostProcessVolume>(TargetActor))
    {
        // Handle actor-level properties on PostProcessVolume (not in Settings struct)
        if (PropertyName == TEXT("bUnbound"))
        {
            PPVolume->bUnbound = PropertyValue->AsBool();
            PPVolume->Modify();
            PPVolume->MarkPackageDirty();

            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("success"), true);
            Result->SetStringField(TEXT("actor"), ActorName);
            Result->SetStringField(TEXT("property"), PropertyName);
            Result->SetBoolField(TEXT("value"), PPVolume->bUnbound);
            Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
            return Result;
        }
        else if (PropertyName == TEXT("Priority"))
        {
            PPVolume->Priority = static_cast<float>(PropertyValue->AsNumber());
            PPVolume->Modify();
            PPVolume->MarkPackageDirty();

            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("success"), true);
            Result->SetStringField(TEXT("actor"), ActorName);
            Result->SetStringField(TEXT("property"), PropertyName);
            Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
            return Result;
        }
        else if (PropertyName == TEXT("BlendRadius"))
        {
            PPVolume->BlendRadius = static_cast<float>(PropertyValue->AsNumber());
            PPVolume->Modify();
            PPVolume->MarkPackageDirty();

            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("success"), true);
            Result->SetStringField(TEXT("actor"), ActorName);
            Result->SetStringField(TEXT("property"), PropertyName);
            Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
            return Result;
        }
        else if (PropertyName == TEXT("BlendWeight"))
        {
            PPVolume->BlendWeight = static_cast<float>(PropertyValue->AsNumber());
            PPVolume->Modify();
            PPVolume->MarkPackageDirty();

            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("success"), true);
            Result->SetStringField(TEXT("actor"), ActorName);
            Result->SetStringField(TEXT("property"), PropertyName);
            Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
            return Result;
        }

        if (PropertyPath.Num() > 0 && PropertyPath[0] == TEXT("Settings"))
        {
            // Access the Settings struct directly
            if (PropertyPath.Num() >= 2)
            {
                FString SettingsPropertyName = PropertyPath[1];

                // Get the Settings property
                FStructProperty* SettingsProp = CastField<FStructProperty>(
                    PPVolume->GetClass()->FindPropertyByName(TEXT("Settings")));

                if (SettingsProp)
                {
                    void* SettingsPtr = SettingsProp->ContainerPtrToValuePtr<void>(PPVolume);
                    UScriptStruct* SettingsStruct = SettingsProp->Struct;

                    // Find the property within Settings
                    FProperty* TargetProperty = SettingsStruct->FindPropertyByName(*SettingsPropertyName);

                    if (TargetProperty)
                    {
                        // Create a temporary wrapper object to set the nested property
                        void* PropertyAddr = TargetProperty->ContainerPtrToValuePtr<void>(SettingsPtr);

                        // Handle the property types directly
                        if (FStructProperty* StructProp = CastField<FStructProperty>(TargetProperty))
                        {
                            FString StructName = StructProp->Struct->GetName();

                            if (StructName == TEXT("Vector4"))
                            {
                                const TSharedPtr<FJsonObject>* JsonObj;
                                if (PropertyValue->TryGetObject(JsonObj))
                                {
                                    FVector4* Vector4Ptr = static_cast<FVector4*>(PropertyAddr);
                                    *Vector4Ptr = GetVector4FromJson(*JsonObj);

                                    // Mark the override flag
                                    FString OverrideFlagName = FString::Printf(TEXT("bOverride_%s"), *SettingsPropertyName);
                                    FBoolProperty* OverrideProp = CastField<FBoolProperty>(
                                        SettingsStruct->FindPropertyByName(*OverrideFlagName));
                                    if (OverrideProp)
                                    {
                                        void* OverrideAddr = OverrideProp->ContainerPtrToValuePtr<void>(SettingsPtr);
                                        OverrideProp->SetPropertyValue(OverrideAddr, true);
                                    }

                                    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
                                    Result->SetBoolField(TEXT("success"), true);
                                    Result->SetStringField(TEXT("actor"), ActorName);
                                    Result->SetStringField(TEXT("property"), PropertyName);
                                    Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
                                    return Result;
                                }
                            }
                            else if (StructName == TEXT("LinearColor"))
                            {
                                const TSharedPtr<FJsonObject>* JsonObj;
                                if (PropertyValue->TryGetObject(JsonObj))
                                {
                                    FLinearColor* ColorPtr = static_cast<FLinearColor*>(PropertyAddr);
                                    *ColorPtr = GetLinearColorFromJson(*JsonObj);

                                    // Mark the override flag
                                    FString OverrideFlagName = FString::Printf(TEXT("bOverride_%s"), *SettingsPropertyName);
                                    FBoolProperty* OverrideProp = CastField<FBoolProperty>(
                                        SettingsStruct->FindPropertyByName(*OverrideFlagName));
                                    if (OverrideProp)
                                    {
                                        void* OverrideAddr = OverrideProp->ContainerPtrToValuePtr<void>(SettingsPtr);
                                        OverrideProp->SetPropertyValue(OverrideAddr, true);
                                    }

                                    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
                                    Result->SetBoolField(TEXT("success"), true);
                                    Result->SetStringField(TEXT("actor"), ActorName);
                                    Result->SetStringField(TEXT("property"), PropertyName);
                                    Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
                                    return Result;
                                }
                            }
                        }
                        else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(TargetProperty))
                        {
                            FloatProp->SetPropertyValue(PropertyAddr, static_cast<float>(PropertyValue->AsNumber()));

                            // Mark the override flag
                            FString OverrideFlagName = FString::Printf(TEXT("bOverride_%s"), *SettingsPropertyName);
                            FBoolProperty* OverrideProp = CastField<FBoolProperty>(
                                SettingsStruct->FindPropertyByName(*OverrideFlagName));
                            if (OverrideProp)
                            {
                                void* OverrideAddr = OverrideProp->ContainerPtrToValuePtr<void>(SettingsPtr);
                                OverrideProp->SetPropertyValue(OverrideAddr, true);
                            }

                            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
                            Result->SetBoolField(TEXT("success"), true);
                            Result->SetStringField(TEXT("actor"), ActorName);
                            Result->SetStringField(TEXT("property"), PropertyName);
                            Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
                            return Result;
                        }
                        else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(TargetProperty))
                        {
                            BoolProp->SetPropertyValue(PropertyAddr, PropertyValue->AsBool());

                            // Mark the override flag
                            FString OverrideFlagName = FString::Printf(TEXT("bOverride_%s"), *SettingsPropertyName);
                            FBoolProperty* OverrideProp = CastField<FBoolProperty>(
                                SettingsStruct->FindPropertyByName(*OverrideFlagName));
                            if (OverrideProp)
                            {
                                void* OverrideAddr = OverrideProp->ContainerPtrToValuePtr<void>(SettingsPtr);
                                OverrideProp->SetPropertyValue(OverrideAddr, true);
                            }

                            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
                            Result->SetBoolField(TEXT("success"), true);
                            Result->SetStringField(TEXT("actor"), ActorName);
                            Result->SetStringField(TEXT("property"), PropertyName);
                            Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));
                            return Result;
                        }
                    }
                    else
                    {
                        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                            FString::Printf(TEXT("Property '%s' not found in PostProcessSettings"), *SettingsPropertyName));
                    }
                }
            }
        }
    }

    // Find the component
    UActorComponent* Component = FindComponentOnActor(TargetActor, ComponentName);

    if (!Component)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Component '%s' not found on actor '%s'"),
                *ComponentName, *ActorName));
    }

    // For simple (non-nested) properties on components
    FString SimplePropertyName = PropertyPath.Num() > 0 ? PropertyPath[0] : PropertyName;

    // Find the property using reflection
    FProperty* Property = Component->GetClass()->FindPropertyByName(*SimplePropertyName);

    if (!Property)
    {
        // Try the actor itself if component doesn't have the property
        Property = TargetActor->GetClass()->FindPropertyByName(*SimplePropertyName);
        if (Property)
        {
            CurrentObject = TargetActor;
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Property '%s' not found on component or actor"), *SimplePropertyName));
        }
    }
    else
    {
        CurrentObject = Component;
    }

    // Navigate through struct properties for dot-notation paths (e.g., "AnimationData.AnimToPlay")
    void* ResolvedAddress = nullptr;
    if (PropertyPath.Num() > 1)
    {
        void* CurrentAddr = Property->ContainerPtrToValuePtr<void>(CurrentObject);

        for (int32 i = 1; i < PropertyPath.Num(); ++i)
        {
            FStructProperty* StructProp = CastField<FStructProperty>(Property);
            if (!StructProp)
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                    FString::Printf(TEXT("Property '%s' is not a struct, cannot navigate into it with dot notation"),
                        *Property->GetName()));
            }

            UScriptStruct* Struct = StructProp->Struct;
            FProperty* InnerProperty = Struct->FindPropertyByName(*PropertyPath[i]);
            if (!InnerProperty)
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                    FString::Printf(TEXT("Property '%s' not found in struct '%s'"),
                        *PropertyPath[i], *Struct->GetName()));
            }

            CurrentAddr = InnerProperty->ContainerPtrToValuePtr<void>(CurrentAddr);
            Property = InnerProperty;
        }

        ResolvedAddress = CurrentAddr;
        UE_LOG(LogTemp, Display, TEXT("Navigated struct path: %s -> leaf property: %s"),
              *PropertyName, *Property->GetName());
    }

    // Set the property value
    FString ErrorMessage;
    if (!SetPropertyValue(CurrentObject, Property, PropertyValue, ErrorMessage, ResolvedAddress))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to set property: %s"), *ErrorMessage));
    }

    // Mark the component/actor as modified
    if (Component)
    {
        Component->MarkRenderStateDirty();
        Component->MarkPackageDirty();
    }
    TargetActor->MarkPackageDirty();

    // Return success
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("actor"), ActorName);
    Result->SetStringField(TEXT("component"), Component ? Component->GetName() : TEXT(""));
    Result->SetStringField(TEXT("property"), PropertyName);
    Result->SetStringField(TEXT("message"), TEXT("Property set successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Get optional component name
    FString ComponentName;
    Params->TryGetStringField(TEXT("component_name"), ComponentName);

    // Find the actor
    AActor* TargetActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Find the component
    UActorComponent* Component = FindComponentOnActor(TargetActor, ComponentName);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("actor"), ActorName);
    Result->SetStringField(TEXT("actor_class"), TargetActor->GetClass()->GetName());

    // List all components
    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    TArray<UActorComponent*> Components;
    TargetActor->GetComponents(Components);

    for (UActorComponent* Comp : Components)
    {
        if (Comp)
        {
            TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
            CompObj->SetStringField(TEXT("name"), Comp->GetName());
            CompObj->SetStringField(TEXT("class"), Comp->GetClass()->GetName());

            // List editable properties for this component
            TArray<TSharedPtr<FJsonValue>> PropertiesArray;
            for (TFieldIterator<FProperty> PropIt(Comp->GetClass()); PropIt; ++PropIt)
            {
                FProperty* Property = *PropIt;
                if (Property && (Property->PropertyFlags & CPF_Edit))
                {
                    TSharedPtr<FJsonObject> PropObj = MakeShared<FJsonObject>();
                    PropObj->SetStringField(TEXT("name"), Property->GetName());
                    PropObj->SetStringField(TEXT("type"), Property->GetCPPType());
                    PropertiesArray.Add(MakeShared<FJsonValueObject>(PropObj));
                }
            }
            CompObj->SetArrayField(TEXT("properties"), PropertiesArray);

            ComponentsArray.Add(MakeShared<FJsonValueObject>(CompObj));
        }
    }

    Result->SetArrayField(TEXT("components"), ComponentsArray);

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString MaterialName;
    if (!Params->TryGetStringField(TEXT("name"), MaterialName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional path (defaults to /Game/Materials/)
    FString MaterialPath = TEXT("/Game/Materials/");
    Params->TryGetStringField(TEXT("path"), MaterialPath);

    // Ensure path ends with /
    if (!MaterialPath.EndsWith(TEXT("/")))
    {
        MaterialPath += TEXT("/");
    }

    // Get optional material properties
    FLinearColor BaseColor(0.8f, 0.8f, 0.8f, 1.0f);
    float Roughness = 0.5f;
    float Metallic = 0.0f;

    if (Params->HasField(TEXT("base_color")))
    {
        const TArray<TSharedPtr<FJsonValue>>* BaseColorArray;
        if (Params->TryGetArrayField(TEXT("base_color"), BaseColorArray) && BaseColorArray->Num() >= 3)
        {
            BaseColor.R = (*BaseColorArray)[0]->AsNumber();
            BaseColor.G = (*BaseColorArray)[1]->AsNumber();
            BaseColor.B = (*BaseColorArray)[2]->AsNumber();
            if (BaseColorArray->Num() >= 4)
            {
                BaseColor.A = (*BaseColorArray)[3]->AsNumber();
            }
        }
    }

    if (Params->HasField(TEXT("roughness")))
    {
        Roughness = Params->GetNumberField(TEXT("roughness"));
    }

    if (Params->HasField(TEXT("metallic")))
    {
        Metallic = Params->GetNumberField(TEXT("metallic"));
    }

    // Create the material package
    FString PackagePath = MaterialPath + MaterialName;
    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package for material"));
    }

    // Create the material using factory
    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
        UMaterial::StaticClass(),
        Package,
        FName(*MaterialName),
        RF_Public | RF_Standalone,
        nullptr,
        GWarn
    ));

    if (!NewMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
    }

    // Create Base Color constant expression
    UMaterialExpressionConstant3Vector* BaseColorExpr = NewObject<UMaterialExpressionConstant3Vector>(NewMaterial);
    BaseColorExpr->Constant = FLinearColor(BaseColor.R, BaseColor.G, BaseColor.B);
    NewMaterial->GetExpressionCollection().AddExpression(BaseColorExpr);
    BaseColorExpr->MaterialExpressionEditorX = -300;
    BaseColorExpr->MaterialExpressionEditorY = 0;

    // Create Roughness constant expression
    UMaterialExpressionConstant* RoughnessExpr = NewObject<UMaterialExpressionConstant>(NewMaterial);
    RoughnessExpr->R = Roughness;
    NewMaterial->GetExpressionCollection().AddExpression(RoughnessExpr);
    RoughnessExpr->MaterialExpressionEditorX = -300;
    RoughnessExpr->MaterialExpressionEditorY = 150;

    // Create Metallic constant expression
    UMaterialExpressionConstant* MetallicExpr = NewObject<UMaterialExpressionConstant>(NewMaterial);
    MetallicExpr->R = Metallic;
    NewMaterial->GetExpressionCollection().AddExpression(MetallicExpr);
    MetallicExpr->MaterialExpressionEditorX = -300;
    MetallicExpr->MaterialExpressionEditorY = 250;

    // Connect expressions to material outputs
    NewMaterial->GetEditorOnlyData()->BaseColor.Expression = BaseColorExpr;
    NewMaterial->GetEditorOnlyData()->Roughness.Expression = RoughnessExpr;
    NewMaterial->GetEditorOnlyData()->Metallic.Expression = MetallicExpr;

    // Compile the material
    NewMaterial->PreEditChange(nullptr);
    NewMaterial->PostEditChange();

    // Mark package dirty and save
    Package->MarkPackageDirty();

    // Notify asset registry
    IAssetRegistry::Get()->AssetCreated(NewMaterial);

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), MaterialName);
    Result->SetStringField(TEXT("path"), PackagePath);
    Result->SetStringField(TEXT("message"), TEXT("Material created successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString InstanceName;
    if (!Params->TryGetStringField(TEXT("name"), InstanceName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString ParentMaterialPath;
    if (!Params->TryGetStringField(TEXT("parent_material"), ParentMaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parent_material' parameter"));
    }

    // Load parent material
    UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(ParentMaterialPath));
    if (!ParentMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Parent material not found: %s"), *ParentMaterialPath));
    }

    // Get optional path (defaults to same path as parent)
    FString InstancePath = FPaths::GetPath(ParentMaterialPath) + TEXT("/");
    Params->TryGetStringField(TEXT("path"), InstancePath);

    if (!InstancePath.EndsWith(TEXT("/")))
    {
        InstancePath += TEXT("/");
    }

    // Create the material instance package
    FString PackagePath = InstancePath + InstanceName;
    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package for material instance"));
    }

    // Create material instance using factory
    UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
    Factory->InitialParent = ParentMaterial;

    UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(
        UMaterialInstanceConstant::StaticClass(),
        Package,
        FName(*InstanceName),
        RF_Public | RF_Standalone,
        nullptr,
        GWarn
    ));

    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material instance"));
    }

    // Set scalar parameters
    const TSharedPtr<FJsonObject>* ScalarParams;
    if (Params->TryGetObjectField(TEXT("scalar_parameters"), ScalarParams))
    {
        for (const auto& Pair : (*ScalarParams)->Values)
        {
            FName ParamName(*Pair.Key);
            float Value = Pair.Value->AsNumber();
            MaterialInstance->SetScalarParameterValueEditorOnly(ParamName, Value);
        }
    }

    // Set vector parameters
    const TSharedPtr<FJsonObject>* VectorParams;
    if (Params->TryGetObjectField(TEXT("vector_parameters"), VectorParams))
    {
        for (const auto& Pair : (*VectorParams)->Values)
        {
            FName ParamName(*Pair.Key);
            const TArray<TSharedPtr<FJsonValue>>* ValueArray;
            if (Pair.Value->TryGetArray(ValueArray) && ValueArray->Num() >= 3)
            {
                FLinearColor Value;
                Value.R = (*ValueArray)[0]->AsNumber();
                Value.G = (*ValueArray)[1]->AsNumber();
                Value.B = (*ValueArray)[2]->AsNumber();
                Value.A = ValueArray->Num() >= 4 ? (*ValueArray)[3]->AsNumber() : 1.0f;
                MaterialInstance->SetVectorParameterValueEditorOnly(ParamName, Value);
            }
        }
    }

    // Mark package dirty
    Package->MarkPackageDirty();

    // Notify asset registry
    IAssetRegistry::Get()->AssetCreated(MaterialInstance);

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), InstanceName);
    Result->SetStringField(TEXT("path"), PackagePath);
    Result->SetStringField(TEXT("parent"), ParentMaterialPath);
    Result->SetStringField(TEXT("message"), TEXT("Material instance created successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetMaterialInstanceParameter(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    FString ParameterName;
    if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
    }

    if (!Params->HasField(TEXT("parameter_value")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_value' parameter"));
    }

    // Load material instance
    UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!MaterialInstance)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material instance not found: %s"), *MaterialPath));
    }

    FName ParamFName(*ParameterName);
    TSharedPtr<FJsonValue> ParameterValue = Params->TryGetField(TEXT("parameter_value"));

    // Determine parameter type and set value
    FString ParameterType;

    // Try as scalar (number)
    if (ParameterValue->Type == EJson::Number)
    {
        float ScalarValue = ParameterValue->AsNumber();
        MaterialInstance->SetScalarParameterValueEditorOnly(ParamFName, ScalarValue);
        ParameterType = TEXT("scalar");
    }
    // Try as vector (array)
    else if (ParameterValue->Type == EJson::Array)
    {
        const TArray<TSharedPtr<FJsonValue>>& ValueArray = ParameterValue->AsArray();
        if (ValueArray.Num() >= 3)
        {
            FLinearColor VectorValue;
            VectorValue.R = ValueArray[0]->AsNumber();
            VectorValue.G = ValueArray[1]->AsNumber();
            VectorValue.B = ValueArray[2]->AsNumber();
            VectorValue.A = ValueArray.Num() >= 4 ? ValueArray[3]->AsNumber() : 1.0f;
            MaterialInstance->SetVectorParameterValueEditorOnly(ParamFName, VectorValue);
            ParameterType = TEXT("vector");
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                TEXT("Vector parameter requires at least 3 values [R, G, B]"));
        }
    }
    // Try as texture (string path)
    else if (ParameterValue->Type == EJson::String)
    {
        FString TexturePath = ParameterValue->AsString();
        UTexture* Texture = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(TexturePath));
        if (Texture)
        {
            MaterialInstance->SetTextureParameterValueEditorOnly(ParamFName, Texture);
            ParameterType = TEXT("texture");
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
                FString::Printf(TEXT("Texture not found: %s"), *TexturePath));
        }
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Unsupported parameter value type"));
    }

    // Mark dirty
    MaterialInstance->MarkPackageDirty();

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("material"), MaterialPath);
    Result->SetStringField(TEXT("parameter"), ParameterName);
    Result->SetStringField(TEXT("type"), ParameterType);
    Result->SetStringField(TEXT("message"), TEXT("Parameter set successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleImportTexture(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString TextureName;
    if (!Params->TryGetStringField(TEXT("texture_name"), TextureName))
    {
        // Use source filename if not provided
        TextureName = FPaths::GetBaseFilename(SourcePath);
    }

    // Get optional destination path
    FString DestinationPath = TEXT("/Game/Textures/");
    Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

    if (!DestinationPath.EndsWith(TEXT("/")))
    {
        DestinationPath += TEXT("/");
    }

    // Check if source file exists
    if (!FPaths::FileExists(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file not found: %s"), *SourcePath));
    }

    // Create package for the texture
    FString PackagePath = DestinationPath + TextureName;

    // If asset already exists, return success — NEVER delete+reimport over loaded assets
    // (editor subsystems hold RefCount > 0, causing "partially loaded" crash on SavePackage)
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("import_texture: Asset '%s' already exists, skipping import"), *PackagePath);
        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("name"), TextureName);
        Result->SetStringField(TEXT("path"), PackagePath);
        Result->SetStringField(TEXT("message"), TEXT("Asset already exists, skipped import"));
        return Result;
    }

    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package for texture"));
    }

    // Create texture factory
    UTextureFactory* TextureFactory = NewObject<UTextureFactory>();
    TextureFactory->AddToRoot();

    // Import the texture
    bool bCancelled = false;
    UTexture2D* ImportedTexture = Cast<UTexture2D>(TextureFactory->ImportObject(
        UTexture2D::StaticClass(),
        Package,
        FName(*TextureName),
        RF_Public | RF_Standalone,
        SourcePath,
        nullptr,
        bCancelled
    ));

    TextureFactory->RemoveFromRoot();

    if (!ImportedTexture)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to import texture from: %s"), *SourcePath));
    }

    // Apply optional texture properties after import
    FString CompressionType;
    if (Params->TryGetStringField(TEXT("compression_type"), CompressionType))
    {
        if (CompressionType == TEXT("Normalmap") || CompressionType == TEXT("TC_Normalmap"))
            ImportedTexture->CompressionSettings = TC_Normalmap;
        else if (CompressionType == TEXT("Masks") || CompressionType == TEXT("TC_Masks"))
            ImportedTexture->CompressionSettings = TC_Masks;
        else if (CompressionType == TEXT("Default") || CompressionType == TEXT("TC_Default"))
            ImportedTexture->CompressionSettings = TC_Default;
        else if (CompressionType == TEXT("Grayscale") || CompressionType == TEXT("TC_Grayscale"))
            ImportedTexture->CompressionSettings = TC_Grayscale;
        else if (CompressionType == TEXT("HDR") || CompressionType == TEXT("TC_HDR"))
            ImportedTexture->CompressionSettings = TC_HDR;
        else if (CompressionType == TEXT("EditorIcon") || CompressionType == TEXT("TC_EditorIcon") || CompressionType == TEXT("UserInterface2D"))
        {
            ImportedTexture->CompressionSettings = TC_EditorIcon;
            // UI textures must be fully resident — no streaming, no mipmaps
            ImportedTexture->NeverStream = true;
            ImportedTexture->MipGenSettings = TMGS_NoMipmaps;
            ImportedTexture->LODGroup = TEXTUREGROUP_UI;
        }
    }

    bool bSRGB;
    if (Params->TryGetBoolField(TEXT("srgb"), bSRGB))
    {
        ImportedTexture->SRGB = bSRGB;
    }

    bool bFlipGreen;
    if (Params->TryGetBoolField(TEXT("flip_green_channel"), bFlipGreen))
    {
        ImportedTexture->bFlipGreenChannel = bFlipGreen;
    }

    ImportedTexture->PostEditChange();
    ImportedTexture->UpdateResource();

    // Notify asset registry
    IAssetRegistry::Get()->AssetCreated(ImportedTexture);

    // CRITICAL: Save package to disk immediately after import.
    // Without this, texture packages accumulate in memory (~64MB per 4K texture).
    // Rapid imports without saving cause memory pressure that triggers GC,
    // which can unload/corrupt landscape streaming proxies.
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, ImportedTexture, *PackageFilename, SaveArgs);

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), TextureName);
    Result->SetStringField(TEXT("path"), PackagePath);
    Result->SetStringField(TEXT("source"), SourcePath);
    Result->SetNumberField(TEXT("width"), ImportedTexture->GetSizeX());
    Result->SetNumberField(TEXT("height"), ImportedTexture->GetSizeY());
    Result->SetStringField(TEXT("message"), TEXT("Texture imported successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetTextureProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString TexturePath;
    if (!Params->TryGetStringField(TEXT("texture_path"), TexturePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'texture_path' parameter"));
    }

    // Load the texture asset
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(TexturePath);
    UTexture2D* Texture = Cast<UTexture2D>(LoadedAsset);
    if (!Texture)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load texture at: %s"), *TexturePath));
    }

    bool bChanged = false;

    // Set compression type
    FString CompressionType;
    if (Params->TryGetStringField(TEXT("compression_type"), CompressionType))
    {
        if (CompressionType == TEXT("Normalmap") || CompressionType == TEXT("TC_Normalmap"))
        {
            Texture->CompressionSettings = TC_Normalmap;
            bChanged = true;
        }
        else if (CompressionType == TEXT("Masks") || CompressionType == TEXT("TC_Masks"))
        {
            Texture->CompressionSettings = TC_Masks;
            bChanged = true;
        }
        else if (CompressionType == TEXT("Default") || CompressionType == TEXT("TC_Default"))
        {
            Texture->CompressionSettings = TC_Default;
            bChanged = true;
        }
        else if (CompressionType == TEXT("Grayscale") || CompressionType == TEXT("TC_Grayscale"))
        {
            Texture->CompressionSettings = TC_Grayscale;
            bChanged = true;
        }
        else if (CompressionType == TEXT("HDR") || CompressionType == TEXT("TC_HDR"))
        {
            Texture->CompressionSettings = TC_HDR;
            bChanged = true;
        }
        else if (CompressionType == TEXT("EditorIcon") || CompressionType == TEXT("TC_EditorIcon") || CompressionType == TEXT("UserInterface2D"))
        {
            Texture->CompressionSettings = TC_EditorIcon;
            // UI textures must be fully resident — no streaming, no mipmaps
            Texture->NeverStream = true;
            Texture->MipGenSettings = TMGS_NoMipmaps;
            Texture->LODGroup = TEXTUREGROUP_UI;
            bChanged = true;
        }
    }

    // Set sRGB
    bool bSRGB;
    if (Params->TryGetBoolField(TEXT("srgb"), bSRGB))
    {
        Texture->SRGB = bSRGB;
        bChanged = true;
    }

    // Flip green channel (for OpenGL → DirectX normal maps)
    bool bFlipGreen;
    if (Params->TryGetBoolField(TEXT("flip_green_channel"), bFlipGreen))
    {
        Texture->bFlipGreenChannel = bFlipGreen;
        bChanged = true;
    }

    // Set NeverStream explicitly (useful for UI textures)
    bool bNeverStream;
    if (Params->TryGetBoolField(TEXT("never_stream"), bNeverStream))
    {
        Texture->NeverStream = bNeverStream;
        bChanged = true;
    }

    if (bChanged)
    {
        Texture->PostEditChange();
        Texture->UpdateResource();
        Texture->MarkPackageDirty();
    }

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("texture_path"), TexturePath);
    Result->SetStringField(TEXT("compression"),
        Texture->CompressionSettings == TC_Normalmap ? TEXT("TC_Normalmap") :
        Texture->CompressionSettings == TC_Masks ? TEXT("TC_Masks") :
        Texture->CompressionSettings == TC_Grayscale ? TEXT("TC_Grayscale") :
        Texture->CompressionSettings == TC_HDR ? TEXT("TC_HDR") :
        Texture->CompressionSettings == TC_EditorIcon ? TEXT("TC_EditorIcon") :
        TEXT("TC_Default"));
    Result->SetBoolField(TEXT("srgb"), Texture->SRGB);
    Result->SetBoolField(TEXT("flip_green_channel"), Texture->bFlipGreenChannel);
    Result->SetBoolField(TEXT("never_stream"), Texture->NeverStream);
    Result->SetStringField(TEXT("message"), TEXT("Texture properties updated successfully"));

    return Result;
}

// Helper function to auto-detect correct sampler type from texture compression settings
static EMaterialSamplerType GetSamplerTypeForTexture(UTexture* Texture)
{
    if (!Texture) return SAMPLERTYPE_LinearColor;

    UTexture2D* Tex2D = Cast<UTexture2D>(Texture);
    if (!Tex2D) return SAMPLERTYPE_LinearColor;

    switch (Tex2D->CompressionSettings)
    {
    case TC_Default:
        return Tex2D->SRGB ? SAMPLERTYPE_Color : SAMPLERTYPE_LinearColor;
    case TC_Normalmap:
        return SAMPLERTYPE_Normal;
    case TC_Masks:
        return SAMPLERTYPE_Masks;
    case TC_Grayscale:
        return SAMPLERTYPE_Grayscale;
    case TC_Alpha:
        return SAMPLERTYPE_Alpha;
    case TC_DistanceFieldFont:
        return SAMPLERTYPE_DistanceFieldFont;
    default:
        return SAMPLERTYPE_LinearColor;
    }
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCreatePBRMaterial(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString MaterialName;
    if (!Params->TryGetStringField(TEXT("name"), MaterialName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString MaterialPath = TEXT("/Game/Materials/");
    Params->TryGetStringField(TEXT("path"), MaterialPath);
    if (!MaterialPath.EndsWith(TEXT("/"))) MaterialPath += TEXT("/");

    // Get texture paths
    FString DiffuseTexturePath, NormalTexturePath, ARMTexturePath;
    Params->TryGetStringField(TEXT("diffuse_texture"), DiffuseTexturePath);
    Params->TryGetStringField(TEXT("normal_texture"), NormalTexturePath);
    Params->TryGetStringField(TEXT("arm_texture"), ARMTexturePath);

    // Optional separate roughness/metallic/ao textures (if not using ARM)
    FString RoughnessTexturePath, MetallicTexturePath, AOTexturePath;
    Params->TryGetStringField(TEXT("roughness_texture"), RoughnessTexturePath);
    Params->TryGetStringField(TEXT("metallic_texture"), MetallicTexturePath);
    Params->TryGetStringField(TEXT("ao_texture"), AOTexturePath);

    // Optional opacity mask texture (automatically sets Masked blend mode)
    FString OpacityMaskTexturePath;
    Params->TryGetStringField(TEXT("opacity_mask_texture"), OpacityMaskTexturePath);

    // Optional scalar values
    double RoughnessValue = 0.5;
    double MetallicValue = 0.0;
    bool bHasRoughnessValue = Params->TryGetNumberField(TEXT("roughness_value"), RoughnessValue);
    bool bHasMetallicValue = Params->TryGetNumberField(TEXT("metallic_value"), MetallicValue);

    // Create material package - delete existing asset first to avoid duplicates
    FString FullPath = MaterialPath + MaterialName;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        UEditorAssetLibrary::DeleteAsset(FullPath);
    }

    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    // Create the material
    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    UMaterial* Material = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
        UMaterial::StaticClass(), Package, FName(*MaterialName),
        RF_Standalone | RF_Public, nullptr, GWarn));

    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
    }

    Material->PreEditChange(nullptr);

    int32 PosY = 0;

    // === BASE COLOR (DIFFUSE) ===
    if (!DiffuseTexturePath.IsEmpty())
    {
        UTexture* DiffuseTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(DiffuseTexturePath));
        if (DiffuseTex)
        {
            UMaterialExpressionTextureSample* DiffuseSample = NewObject<UMaterialExpressionTextureSample>(Material);
            DiffuseSample->Texture = DiffuseTex;
            DiffuseSample->SamplerType = SAMPLERTYPE_Color;
            DiffuseSample->MaterialExpressionEditorX = -400;
            DiffuseSample->MaterialExpressionEditorY = PosY;
            Material->GetExpressionCollection().AddExpression(DiffuseSample);
            Material->GetEditorOnlyData()->BaseColor.Connect(0, DiffuseSample);
            PosY += 300;
        }
    }

    // === NORMAL MAP ===
    if (!NormalTexturePath.IsEmpty())
    {
        UTexture* NormalTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(NormalTexturePath));
        if (NormalTex)
        {
            UMaterialExpressionTextureSample* NormalSample = NewObject<UMaterialExpressionTextureSample>(Material);
            NormalSample->Texture = NormalTex;
            NormalSample->SamplerType = SAMPLERTYPE_Normal;
            NormalSample->MaterialExpressionEditorX = -400;
            NormalSample->MaterialExpressionEditorY = PosY;
            Material->GetExpressionCollection().AddExpression(NormalSample);
            Material->GetEditorOnlyData()->Normal.Connect(0, NormalSample);
            PosY += 300;
        }
    }

    // === ARM PACKED TEXTURE (AO=R, Roughness=G, Metallic=B) ===
    if (!ARMTexturePath.IsEmpty())
    {
        UTexture* ARMTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(ARMTexturePath));
        if (ARMTex)
        {
            UMaterialExpressionTextureSample* ARMSample = NewObject<UMaterialExpressionTextureSample>(Material);
            ARMSample->Texture = ARMTex;
            ARMSample->SamplerType = SAMPLERTYPE_Masks;
            ARMSample->MaterialExpressionEditorX = -400;
            ARMSample->MaterialExpressionEditorY = PosY;
            Material->GetExpressionCollection().AddExpression(ARMSample);

            // AO: NEVER connect from ARM texture. ARM textures from Megascans/PolyHaven
            // have AO=0 in UV padding areas, causing dark patches. UE5 Lumen handles
            // ambient occlusion automatically, making texture AO unnecessary and harmful.

            // Roughness = Green channel
            UMaterialExpressionComponentMask* RoughnessMask = NewObject<UMaterialExpressionComponentMask>(Material);
            RoughnessMask->R = false; RoughnessMask->G = true; RoughnessMask->B = false; RoughnessMask->A = false;
            RoughnessMask->Input.Connect(0, ARMSample);
            RoughnessMask->MaterialExpressionEditorX = -100;
            RoughnessMask->MaterialExpressionEditorY = PosY + 80;
            Material->GetExpressionCollection().AddExpression(RoughnessMask);
            Material->GetEditorOnlyData()->Roughness.Connect(0, RoughnessMask);

            // Metallic = Blue channel
            UMaterialExpressionComponentMask* MetallicMask = NewObject<UMaterialExpressionComponentMask>(Material);
            MetallicMask->R = false; MetallicMask->G = false; MetallicMask->B = true; MetallicMask->A = false;
            MetallicMask->Input.Connect(0, ARMSample);
            MetallicMask->MaterialExpressionEditorX = -100;
            MetallicMask->MaterialExpressionEditorY = PosY + 160;
            Material->GetExpressionCollection().AddExpression(MetallicMask);
            Material->GetEditorOnlyData()->Metallic.Connect(0, MetallicMask);

            PosY += 400;
        }
    }
    else
    {
        // Separate roughness/metallic/ao textures or scalar values
        if (!RoughnessTexturePath.IsEmpty())
        {
            UTexture* RoughTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(RoughnessTexturePath));
            if (RoughTex)
            {
                UMaterialExpressionTextureSample* RoughSample = NewObject<UMaterialExpressionTextureSample>(Material);
                RoughSample->Texture = RoughTex;
                RoughSample->SamplerType = GetSamplerTypeForTexture(RoughTex);
                RoughSample->MaterialExpressionEditorX = -400;
                RoughSample->MaterialExpressionEditorY = PosY;
                Material->GetExpressionCollection().AddExpression(RoughSample);
                Material->GetEditorOnlyData()->Roughness.Connect(0, RoughSample);
                PosY += 300;
            }
        }
        else if (bHasRoughnessValue)
        {
            UMaterialExpressionConstant* RoughConst = NewObject<UMaterialExpressionConstant>(Material);
            RoughConst->R = RoughnessValue;
            RoughConst->MaterialExpressionEditorX = -200;
            RoughConst->MaterialExpressionEditorY = PosY;
            Material->GetExpressionCollection().AddExpression(RoughConst);
            Material->GetEditorOnlyData()->Roughness.Connect(0, RoughConst);
            PosY += 100;
        }

        if (!MetallicTexturePath.IsEmpty())
        {
            UTexture* MetTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(MetallicTexturePath));
            if (MetTex)
            {
                UMaterialExpressionTextureSample* MetSample = NewObject<UMaterialExpressionTextureSample>(Material);
                MetSample->Texture = MetTex;
                MetSample->SamplerType = GetSamplerTypeForTexture(MetTex);
                MetSample->MaterialExpressionEditorX = -400;
                MetSample->MaterialExpressionEditorY = PosY;
                Material->GetExpressionCollection().AddExpression(MetSample);
                Material->GetEditorOnlyData()->Metallic.Connect(0, MetSample);
                PosY += 300;
            }
        }
        else if (bHasMetallicValue)
        {
            UMaterialExpressionConstant* MetConst = NewObject<UMaterialExpressionConstant>(Material);
            MetConst->R = MetallicValue;
            MetConst->MaterialExpressionEditorX = -200;
            MetConst->MaterialExpressionEditorY = PosY;
            Material->GetExpressionCollection().AddExpression(MetConst);
            Material->GetEditorOnlyData()->Metallic.Connect(0, MetConst);
            PosY += 100;
        }

        if (!AOTexturePath.IsEmpty())
        {
            UTexture* AOTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(AOTexturePath));
            if (AOTex)
            {
                UMaterialExpressionTextureSample* AOSample = NewObject<UMaterialExpressionTextureSample>(Material);
                AOSample->Texture = AOTex;
                AOSample->SamplerType = GetSamplerTypeForTexture(AOTex);
                AOSample->MaterialExpressionEditorX = -400;
                AOSample->MaterialExpressionEditorY = PosY;
                Material->GetExpressionCollection().AddExpression(AOSample);
                Material->GetEditorOnlyData()->AmbientOcclusion.Connect(0, AOSample);
                PosY += 300;
            }
        }
    }

    // === OPACITY MASK (for foliage/grass alpha cutout) ===
    if (!OpacityMaskTexturePath.IsEmpty())
    {
        UTexture* OpacityTex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(OpacityMaskTexturePath));
        if (OpacityTex)
        {
            UMaterialExpressionTextureSample* OpacitySample = NewObject<UMaterialExpressionTextureSample>(Material);
            OpacitySample->Texture = OpacityTex;
            OpacitySample->SamplerType = SAMPLERTYPE_Masks;
            OpacitySample->MaterialExpressionEditorX = -400;
            OpacitySample->MaterialExpressionEditorY = PosY;
            Material->GetExpressionCollection().AddExpression(OpacitySample);
            Material->GetEditorOnlyData()->OpacityMask.Connect(0, OpacitySample);
            PosY += 300;
        }

        // Set blend mode to Masked when opacity mask is provided
        Material->BlendMode = BLEND_Masked;
    }

    // Optional explicit blend mode override
    FString BlendModeStr;
    if (Params->TryGetStringField(TEXT("blend_mode"), BlendModeStr))
    {
        if (BlendModeStr == TEXT("Opaque")) Material->BlendMode = BLEND_Opaque;
        else if (BlendModeStr == TEXT("Masked")) Material->BlendMode = BLEND_Masked;
        else if (BlendModeStr == TEXT("Translucent")) Material->BlendMode = BLEND_Translucent;
        else if (BlendModeStr == TEXT("Additive")) Material->BlendMode = BLEND_Additive;
    }

    // Set two-sided rendering (fixes flipped normals showing as black)
    bool bTwoSided = false;
    if (Params->TryGetBoolField(TEXT("two_sided"), bTwoSided) && bTwoSided)
    {
        Material->TwoSided = true;
    }

    // Finalize material
    Material->PostEditChange();
    Package->MarkPackageDirty();
    IAssetRegistry::Get()->AssetCreated(Material);

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), MaterialName);
    Result->SetStringField(TEXT("path"), FullPath);
    Result->SetNumberField(TEXT("expression_count"), Material->GetExpressionCollection().Expressions.Num());
    Result->SetStringField(TEXT("message"), TEXT("PBR material created successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleCreateLandscapeMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialName;
    if (!Params->TryGetStringField(TEXT("name"), MaterialName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FString MaterialPath = TEXT("/Game/Materials/");
    Params->TryGetStringField(TEXT("path"), MaterialPath);
    if (!MaterialPath.EndsWith(TEXT("/"))) MaterialPath += TEXT("/");

    // Texture paths
    FString RockD, RockN, MudD, MudN, GrassD, GrassNPath, MudDetailD;
    Params->TryGetStringField(TEXT("rock_d"), RockD);
    Params->TryGetStringField(TEXT("rock_n"), RockN);
    Params->TryGetStringField(TEXT("mud_d"), MudD);
    Params->TryGetStringField(TEXT("mud_n"), MudN);
    Params->TryGetStringField(TEXT("grass_d"), GrassD);
    Params->TryGetStringField(TEXT("grass_n"), GrassNPath);
    Params->TryGetStringField(TEXT("mud_detail_d"), MudDetailD);

    // Scalar parameters — UV Noise Distortion + Macro Variation approach
    double DetailUVScale = 0.004;
    double WarpScale = 0.00005;     // Low-frequency warp noise (large features)
    double WarpAmount = 0.05;       // How much UVs are distorted (in UV space)
    double MacroScale = 0.00003;    // Very low-frequency brightness variation
    double MacroStrength = 0.4;     // Brightness modulation amount 0-1
    double SlopeSharpness = 3.0;
    double GrassAmount = 0.5;
    double RoughnessVal = 0.85;
    double MudAmount = 0.3;
    double PuddleAmount = 0.2;
    Params->TryGetNumberField(TEXT("detail_uv_scale"), DetailUVScale);
    Params->TryGetNumberField(TEXT("warp_scale"), WarpScale);
    Params->TryGetNumberField(TEXT("warp_amount"), WarpAmount);
    Params->TryGetNumberField(TEXT("macro_scale"), MacroScale);
    Params->TryGetNumberField(TEXT("macro_strength"), MacroStrength);
    Params->TryGetNumberField(TEXT("slope_sharpness"), SlopeSharpness);
    Params->TryGetNumberField(TEXT("grass_amount"), GrassAmount);
    Params->TryGetNumberField(TEXT("roughness"), RoughnessVal);
    Params->TryGetNumberField(TEXT("mud_amount"), MudAmount);
    Params->TryGetNumberField(TEXT("puddle_amount"), PuddleAmount);

    double HeightBlendStrength = 0.5;
    double PuddleHeightBias = 1.0;
    double RubbleAmount = 0.3;
    double StoneAmount = 0.2;
    Params->TryGetNumberField(TEXT("height_blend_strength"), HeightBlendStrength);
    Params->TryGetNumberField(TEXT("puddle_height_bias"), PuddleHeightBias);
    Params->TryGetNumberField(TEXT("rubble_amount"), RubbleAmount);
    Params->TryGetNumberField(TEXT("stone_amount"), StoneAmount);

    // Create material package
    FString FullPath = MaterialPath + MaterialName;
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        UEditorAssetLibrary::DeleteAsset(FullPath);
    }

    UPackage* Package = CreatePackage(*FullPath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    UMaterial* Mat = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
        UMaterial::StaticClass(), Package, FName(*MaterialName),
        RF_Standalone | RF_Public, nullptr, GWarn));

    if (!Mat)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
    }

    Mat->PreEditChange(nullptr);

    auto LoadTex = [](const FString& Path) -> UTexture* {
        if (Path.IsEmpty()) return nullptr;
        return Cast<UTexture>(UEditorAssetLibrary::LoadAsset(Path));
    };

    auto AddExpr = [Mat](UMaterialExpression* Expr, float X, float Y) {
        Expr->MaterialExpressionEditorX = X;
        Expr->MaterialExpressionEditorY = Y;
        Mat->GetExpressionCollection().AddExpression(Expr);
        return Expr;
    };

    // Comment box lambda for organized graph layout
    auto AddComment = [Mat](const FString& Label, FLinearColor Color, float X, float Y, int32 W, int32 H) {
        auto* C = NewObject<UMaterialExpressionComment>(Mat);
        C->Text = Label;
        C->CommentColor = Color;
        C->FontSize = 18;
        C->MaterialExpressionEditorX = X;
        C->MaterialExpressionEditorY = Y;
        C->SizeX = W;
        C->SizeY = H;
        Mat->GetExpressionCollection().AddComment(C);
    };

    auto CreateTexSample = [&](UTexture* Tex, EMaterialSamplerType SamplerType,
                               UMaterialExpression* UV, float X, float Y) -> UMaterialExpressionTextureSample* {
        if (!Tex) return nullptr;
        auto* S = NewObject<UMaterialExpressionTextureSample>(Mat);
        S->Texture = Tex;
        S->SamplerType = SamplerType;
        S->Coordinates.Connect(0, UV);
        AddExpr(S, X, Y);
        return S;
    };

    // =================================================================
    // COMMENT BOX 1: UV Generation (Yellow)
    // =================================================================
    AddComment(TEXT("1. World Position -> Texture Coordinates"), FLinearColor(0.8f, 0.7f, 0.1f), -2600, -300, 700, 400);

    // =================================================================
    // SECTION 1: Base UVs (3 nodes)
    // WorldPos -> MaskXY(R,G) -> Multiply(DetailScale) -> BaseUV
    // =================================================================
    auto* WorldPos = NewObject<UMaterialExpressionWorldPosition>(Mat);
    AddExpr(WorldPos, -2500, -200);

    auto* MaskRG = NewObject<UMaterialExpressionComponentMask>(Mat);
    MaskRG->R = true; MaskRG->G = true; MaskRG->B = false; MaskRG->A = false;
    MaskRG->Input.Connect(0, WorldPos);
    AddExpr(MaskRG, -2200, -200);

    auto* DetailScaleConst = NewObject<UMaterialExpressionScalarParameter>(Mat);
    DetailScaleConst->ParameterName = FName(TEXT("DetailUVScale"));
    DetailScaleConst->DefaultValue = DetailUVScale;
    AddExpr(DetailScaleConst, -2200, -50);

    auto* BaseUV = NewObject<UMaterialExpressionMultiply>(Mat);
    BaseUV->A.Connect(0, MaskRG);
    BaseUV->B.Connect(0, DetailScaleConst);
    AddExpr(BaseUV, -2000, -200);

    // =================================================================
    // SECTION 1B: Distance-Based Tiling Fade (6 nodes)
    // Fade UV warp and rotation dissolve at far distances to hide tiling
    // =================================================================
    AddComment(TEXT("2. Camera Distance Fade (hide tiling at range)"), FLinearColor(0.4f, 0.8f, 0.4f), -2600, 100, 700, 500);

    // Camera position
    auto* CamPos = NewObject<UMaterialExpressionCameraPositionWS>(Mat);
    AddExpr(CamPos, -2500, 200);

    // Distance = Length(CamPos - WorldPos)
    auto* CamDist = NewObject<UMaterialExpressionDistance>(Mat);
    CamDist->A.Connect(0, CamPos);
    CamDist->B.Connect(0, WorldPos);
    AddExpr(CamDist, -2100, 200);

    // Normalize: distance / 50000 (full fade at 500 meters)
    auto* DistDivConst = NewObject<UMaterialExpressionConstant>(Mat);
    DistDivConst->R = 50000.0f;
    AddExpr(DistDivConst, -2100, 350);

    auto* DistNorm = NewObject<UMaterialExpressionDivide>(Mat);
    DistNorm->A.Connect(0, CamDist);
    DistNorm->B.Connect(0, DistDivConst);
    AddExpr(DistNorm, -1900, 200);

    // Clamp to 0-1
    auto* DistFade = NewObject<UMaterialExpressionClamp>(Mat);
    DistFade->Input.Connect(0, DistNorm);
    DistFade->MinDefault = 0.0f;
    DistFade->MaxDefault = 1.0f;
    AddExpr(DistFade, -1700, 200);

    // =================================================================
    // COMMENT BOX 2: UV Noise Distortion (Orange)
    // =================================================================
    AddComment(TEXT("3. Anti-Tiling: UV Warping"), FLinearColor(0.9f, 0.5f, 0.1f), -2100, 300, 1200, 700);

    // =================================================================
    // SECTION 2: UV Distortion (~10 nodes)
    // Two noise nodes sample WorldPos at WarpScale for X and Y warp.
    // WarpNoiseY uses an offset WorldPos for a different pattern.
    // DistortedUV = BaseUV + AppendVector(WarpX, WarpY) * WarpAmount
    // =================================================================

    // WarpNoiseX: Noise(WorldPos, WarpScale)
    auto* WarpNoiseX = NewObject<UMaterialExpressionNoise>(Mat);
    WarpNoiseX->NoiseFunction = NOISEFUNCTION_GradientALU;
    WarpNoiseX->Scale = WarpScale;
    WarpNoiseX->Quality = 2;
    WarpNoiseX->Levels = 4;
    WarpNoiseX->OutputMin = -1.0f;
    WarpNoiseX->OutputMax = 1.0f;
    WarpNoiseX->bTurbulence = false;
    WarpNoiseX->bTiling = false;
    WarpNoiseX->LevelScale = 2.0f;
    WarpNoiseX->Position.Connect(0, WorldPos);
    AddExpr(WarpNoiseX, -2000, 400);

    // Offset for WarpNoiseY: Add(WorldPos, Constant3Vector(1000, 2000, 0))
    auto* WarpOffsetVec = NewObject<UMaterialExpressionConstant3Vector>(Mat);
    WarpOffsetVec->Constant = FLinearColor(1000.0f, 2000.0f, 0.0f, 0.0f);
    AddExpr(WarpOffsetVec, -2000, 700);

    auto* WarpPosOffset = NewObject<UMaterialExpressionAdd>(Mat);
    WarpPosOffset->A.Connect(0, WorldPos);
    WarpPosOffset->B.Connect(0, WarpOffsetVec);
    AddExpr(WarpPosOffset, -1800, 700);

    // WarpNoiseY: Noise(WorldPos + offset, WarpScale)
    auto* WarpNoiseY = NewObject<UMaterialExpressionNoise>(Mat);
    WarpNoiseY->NoiseFunction = NOISEFUNCTION_GradientALU;
    WarpNoiseY->Scale = WarpScale;
    WarpNoiseY->Quality = 2;
    WarpNoiseY->Levels = 4;
    WarpNoiseY->OutputMin = -1.0f;
    WarpNoiseY->OutputMax = 1.0f;
    WarpNoiseY->bTurbulence = false;
    WarpNoiseY->bTiling = false;
    WarpNoiseY->LevelScale = 2.0f;
    WarpNoiseY->Position.Connect(0, WarpPosOffset);
    AddExpr(WarpNoiseY, -1600, 700);

    // WarpAmount parameter (exposed in MI)
    auto* WarpAmountConst = NewObject<UMaterialExpressionScalarParameter>(Mat);
    WarpAmountConst->ParameterName = FName(TEXT("WarpAmount"));
    WarpAmountConst->DefaultValue = WarpAmount;
    AddExpr(WarpAmountConst, -1600, 500);

    // Distance-modulated warp: Lerp(WarpAmount, WarpAmount*0.3, DistFade)
    // At close range (DistFade=0): full warp. At far range (DistFade=1): 30% warp
    auto* WarpDistMin = NewObject<UMaterialExpressionConstant>(Mat);
    WarpDistMin->R = 0.3f;
    AddExpr(WarpDistMin, -1600, 450);

    auto* WarpReduced = NewObject<UMaterialExpressionMultiply>(Mat);
    WarpReduced->A.Connect(0, WarpAmountConst);
    WarpReduced->B.Connect(0, WarpDistMin);
    AddExpr(WarpReduced, -1400, 450);

    auto* EffectiveWarp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
    EffectiveWarp->A.Connect(0, WarpAmountConst);
    EffectiveWarp->B.Connect(0, WarpReduced);
    EffectiveWarp->Alpha.Connect(0, DistFade);
    AddExpr(EffectiveWarp, -1200, 450);

    // WarpX = Multiply(WarpNoiseX, EffectiveWarp)
    auto* WarpX = NewObject<UMaterialExpressionMultiply>(Mat);
    WarpX->A.Connect(0, WarpNoiseX);
    WarpX->B.Connect(0, EffectiveWarp);
    AddExpr(WarpX, -1400, 400);

    // WarpY = Multiply(WarpNoiseY, EffectiveWarp)
    auto* WarpY = NewObject<UMaterialExpressionMultiply>(Mat);
    WarpY->A.Connect(0, WarpNoiseY);
    WarpY->B.Connect(0, EffectiveWarp);
    AddExpr(WarpY, -1400, 700);

    // AppendWarp = AppendVector(WarpX, WarpY) -> float2
    auto* AppendWarp = NewObject<UMaterialExpressionAppendVector>(Mat);
    AppendWarp->A.Connect(0, WarpX);
    AppendWarp->B.Connect(0, WarpY);
    AddExpr(AppendWarp, -1200, 500);

    // DistortedUV = Add(BaseUV, AppendWarp)
    auto* DistortedUV = NewObject<UMaterialExpressionAdd>(Mat);
    DistortedUV->A.Connect(0, BaseUV);
    DistortedUV->B.Connect(0, AppendWarp);
    AddExpr(DistortedUV, -1000, 400);

    // =================================================================
    // COMMENT BOX 2B: Fixed-Angle Rotation + Dissolve (Pink)
    // Sample texture at original UVs AND at a FIXED 37.5° rotated + offset
    // UVs, then dissolve between them with noise. Fixed angle avoids the
    // swirl artifacts that per-pixel noise rotation creates.
    // =================================================================
    AddComment(TEXT("4. Anti-Tiling: Rotated Sample Blend"), FLinearColor(0.9f, 0.3f, 0.6f), -800, 1600, 1200, 800);

    // Fixed rotation: 37.5 degrees (irrational w.r.t. 90° → never aligns with tile grid)
    // sin(37.5°) ≈ 0.6088,  cos(37.5°) ≈ 0.7934
    auto* ConstSin = NewObject<UMaterialExpressionConstant>(Mat);
    ConstSin->R = 0.6088f;
    AddExpr(ConstSin, -500, 1700);

    auto* ConstCos = NewObject<UMaterialExpressionConstant>(Mat);
    ConstCos->R = 0.7934f;
    AddExpr(ConstCos, -500, 1850);

    // Split DistortedUV into U and V
    auto* RotMaskU = NewObject<UMaterialExpressionComponentMask>(Mat);
    RotMaskU->R = true; RotMaskU->G = false; RotMaskU->B = false; RotMaskU->A = false;
    RotMaskU->Input.Connect(0, DistortedUV);
    AddExpr(RotMaskU, -300, 1700);

    auto* RotMaskV = NewObject<UMaterialExpressionComponentMask>(Mat);
    RotMaskV->R = false; RotMaskV->G = true; RotMaskV->B = false; RotMaskV->A = false;
    RotMaskV->Input.Connect(0, DistortedUV);
    AddExpr(RotMaskV, -300, 1850);

    // RotU = U*cos - V*sin
    auto* UCos = NewObject<UMaterialExpressionMultiply>(Mat);
    UCos->A.Connect(0, RotMaskU);  UCos->B.Connect(0, ConstCos);
    AddExpr(UCos, -100, 1700);

    auto* VSin = NewObject<UMaterialExpressionMultiply>(Mat);
    VSin->A.Connect(0, RotMaskV);  VSin->B.Connect(0, ConstSin);
    AddExpr(VSin, -100, 1800);

    auto* RotU = NewObject<UMaterialExpressionSubtract>(Mat);
    RotU->A.Connect(0, UCos);  RotU->B.Connect(0, VSin);
    AddExpr(RotU, 100, 1750);

    // RotV = U*sin + V*cos
    auto* USin = NewObject<UMaterialExpressionMultiply>(Mat);
    USin->A.Connect(0, RotMaskU);  USin->B.Connect(0, ConstSin);
    AddExpr(USin, -100, 1950);

    auto* VCos = NewObject<UMaterialExpressionMultiply>(Mat);
    VCos->A.Connect(0, RotMaskV);  VCos->B.Connect(0, ConstCos);
    AddExpr(VCos, -100, 2050);

    auto* RotV = NewObject<UMaterialExpressionAdd>(Mat);
    RotV->A.Connect(0, USin);  RotV->B.Connect(0, VCos);
    AddExpr(RotV, 100, 2000);

    // Add fixed UV offset to decorrelate rotated grid from original
    auto* RotUVRaw = NewObject<UMaterialExpressionAppendVector>(Mat);
    RotUVRaw->A.Connect(0, RotU);  RotUVRaw->B.Connect(0, RotV);
    AddExpr(RotUVRaw, 300, 1850);

    auto* UVOffsetConst = NewObject<UMaterialExpressionConstant2Vector>(Mat);
    UVOffsetConst->R = 0.5f;  UVOffsetConst->G = 0.5f;  // Half-tile offset
    AddExpr(UVOffsetConst, 300, 2000);

    auto* RotatedUV = NewObject<UMaterialExpressionAdd>(Mat);
    RotatedUV->A.Connect(0, RotUVRaw);  RotatedUV->B.Connect(0, UVOffsetConst);
    AddExpr(RotatedUV, 500, 1900);

    // Dissolve blend noise — low frequency for large smooth patches
    auto* BlendPosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
    BlendPosOffset->Constant = FLinearColor(3000.0f, 5000.0f, 0.0f, 0.0f);
    AddExpr(BlendPosOffset, -700, 2200);

    auto* BlendPosAdd = NewObject<UMaterialExpressionAdd>(Mat);
    BlendPosAdd->A.Connect(0, WorldPos);  BlendPosAdd->B.Connect(0, BlendPosOffset);
    AddExpr(BlendPosAdd, -500, 2200);

    auto* BlendNoise = NewObject<UMaterialExpressionNoise>(Mat);
    BlendNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
    BlendNoise->Scale = 0.0003f;  // Very low freq → large blend patches (~3300 units)
    BlendNoise->Quality = 1;
    BlendNoise->Levels = 3;
    BlendNoise->OutputMin = 0.0f;
    BlendNoise->OutputMax = 1.0f;
    BlendNoise->bTurbulence = false;
    BlendNoise->bTiling = false;
    BlendNoise->LevelScale = 2.0f;
    BlendNoise->Position.Connect(0, BlendPosAdd);
    AddExpr(BlendNoise, -300, 2200);

    // At far distance, reduce rotation dissolve effect (bias toward original orientation)
    auto* BlendDistHalf = NewObject<UMaterialExpressionConstant>(Mat);
    BlendDistHalf->R = 0.5f;
    AddExpr(BlendDistHalf, -100, 2350);

    auto* DistBlendScale = NewObject<UMaterialExpressionMultiply>(Mat);
    DistBlendScale->A.Connect(0, DistFade);
    DistBlendScale->B.Connect(0, BlendDistHalf);
    AddExpr(DistBlendScale, 100, 2350);

    auto* DistBlendInv = NewObject<UMaterialExpressionOneMinus>(Mat);
    DistBlendInv->Input.Connect(0, DistBlendScale);
    AddExpr(DistBlendInv, 300, 2350);

    auto* EffectiveBlendNoise = NewObject<UMaterialExpressionMultiply>(Mat);
    EffectiveBlendNoise->A.Connect(0, BlendNoise);
    EffectiveBlendNoise->B.Connect(0, DistBlendInv);
    AddExpr(EffectiveBlendNoise, 500, 2300);

    // FinalBlendNoise: distance-modulated dissolve noise used by all layers
    UMaterialExpression* FinalBlendNoise = EffectiveBlendNoise;

    // =================================================================
    // COMMENT BOX 3: Macro Brightness Variation (Cyan)
    // =================================================================
    AddComment(TEXT("5. Large-Scale Brightness Variation"), FLinearColor(0.1f, 0.7f, 0.8f), -1600, 2500, 700, 400);

    // =================================================================
    // SECTION 3: Macro Variation Noise (4 nodes)
    // MacroNoise(WorldPos, MacroScale) -> Lerp(1.0, MacroNoise, MacroStrength) -> MacroMod
    // =================================================================
    auto* MacroNoise = NewObject<UMaterialExpressionNoise>(Mat);
    MacroNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
    MacroNoise->Scale = MacroScale;
    MacroNoise->Quality = 2;
    MacroNoise->Levels = 4;
    MacroNoise->OutputMin = 0.5f;
    MacroNoise->OutputMax = 1.0f;
    MacroNoise->bTurbulence = false;
    MacroNoise->bTiling = false;
    MacroNoise->LevelScale = 2.0f;
    MacroNoise->Position.Connect(0, WorldPos);
    AddExpr(MacroNoise, -1500, 1200);

    auto* MacroStrengthParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
    MacroStrengthParam->ParameterName = FName(TEXT("MacroStrength"));
    MacroStrengthParam->DefaultValue = MacroStrength;
    AddExpr(MacroStrengthParam, -1500, 1400);

    auto* OneConst = NewObject<UMaterialExpressionConstant>(Mat);
    OneConst->R = 1.0f;
    AddExpr(OneConst, -1300, 1200);

    auto* MacroMod = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
    MacroMod->A.Connect(0, OneConst);
    MacroMod->B.Connect(0, MacroNoise);
    MacroMod->Alpha.Connect(0, MacroStrengthParam);
    AddExpr(MacroMod, -1100, 1300);

    // =================================================================
    // Load textures
    // =================================================================
    UTexture* TexRockD = LoadTex(RockD);
    UTexture* TexRockN = LoadTex(RockN);
    UTexture* TexMudD = LoadTex(MudD);
    UTexture* TexMudN = LoadTex(MudN);
    UTexture* TexGrassD = LoadTex(GrassD);
    UTexture* TexGrassN = LoadTex(GrassNPath);

    // =================================================================
    // SECTION 4: Per-Layer Textures with Rotation + Dissolve Blending
    // Each layer: sample at DistortedUV AND RotatedUV, dissolve blend,
    // then apply MacroMod. 4 samplers per layer = 12 total (of 16 max).
    // =================================================================
    struct FLayerResult {
        UMaterialExpression* Diffuse = nullptr;
        UMaterialExpression* Normal = nullptr;
    };

    auto BuildRotBlendLayer = [&](UTexture* DiffTex, UTexture* NormTex, float BaseY) -> FLayerResult {
        FLayerResult Result;
        if (!DiffTex) return Result;

        // Sample diffuse at original and rotated UVs
        auto* DiffOrig = CreateTexSample(DiffTex, SAMPLERTYPE_Color, DistortedUV, -900, BaseY);
        auto* DiffRot  = CreateTexSample(DiffTex, SAMPLERTYPE_Color, RotatedUV,   -900, BaseY + 120);

        if (DiffOrig && DiffRot)
        {
            // Dissolve blend between orientations
            auto* DiffBlend = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
            DiffBlend->A.Connect(0, DiffOrig);
            DiffBlend->B.Connect(0, DiffRot);
            DiffBlend->Alpha.Connect(0, FinalBlendNoise);
            AddExpr(DiffBlend, -600, BaseY + 60);

            // Apply macro brightness modulation
            auto* MacroDiff = NewObject<UMaterialExpressionMultiply>(Mat);
            MacroDiff->A.Connect(0, DiffBlend);
            MacroDiff->B.Connect(0, MacroMod);
            AddExpr(MacroDiff, -400, BaseY + 60);
            Result.Diffuse = MacroDiff;
        }
        else if (DiffOrig)
        {
            auto* MacroDiff = NewObject<UMaterialExpressionMultiply>(Mat);
            MacroDiff->A.Connect(0, DiffOrig);
            MacroDiff->B.Connect(0, MacroMod);
            AddExpr(MacroDiff, -400, BaseY);
            Result.Diffuse = MacroDiff;
        }

        // Sample normals at original and rotated UVs
        if (NormTex)
        {
            auto* NormOrig = CreateTexSample(NormTex, SAMPLERTYPE_Normal, DistortedUV, -900, BaseY + 280);
            auto* NormRot  = CreateTexSample(NormTex, SAMPLERTYPE_Normal, RotatedUV,   -900, BaseY + 400);

            if (NormOrig && NormRot)
            {
                auto* NormBlend = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
                NormBlend->A.Connect(0, NormOrig);
                NormBlend->B.Connect(0, NormRot);
                NormBlend->Alpha.Connect(0, FinalBlendNoise);
                AddExpr(NormBlend, -600, BaseY + 340);
                Result.Normal = NormBlend;
            }
            else if (NormOrig)
            {
                Result.Normal = NormOrig;
            }
        }

        return Result;
    };

    // COMMENT BOX 4: Rock Layer (Red) - 4 samplers (orig+rot D, orig+rot N)
    AddComment(TEXT("6. Rock Texture (steep slopes)"), FLinearColor(0.8f, 0.2f, 0.2f), -1000, -1100, 700, 700);
    FLayerResult RockLayer = BuildRotBlendLayer(TexRockD, TexRockN, -1000);

    // COMMENT BOX 5: Mud Layer (Brown) - 4 samplers
    AddComment(TEXT("7. Mud/Earth Texture (flat ground)"), FLinearColor(0.6f, 0.4f, 0.2f), -1000, -200, 700, 700);
    FLayerResult MudLayer = BuildRotBlendLayer(TexMudD, TexMudN, -100);

    // COMMENT BOX 6: Grass Layer (Green) - 4 samplers
    AddComment(TEXT("8. Grass Texture (overlay patches)"), FLinearColor(0.2f, 0.7f, 0.2f), -1000, 600, 700, 700);
    FLayerResult GrassLayer = BuildRotBlendLayer(TexGrassD, TexGrassN, 700);

    // =================================================================
    // COMMENT BOX 7: Slope Detection + Outputs (Purple)
    // =================================================================
    AddComment(TEXT("9. Slope Detection -> Layer Blending -> Output"), FLinearColor(0.5f, 0.2f, 0.7f), -400, -1700, 1500, 4000);

    // =================================================================
    // SECTION 5: Slope Detection (5 nodes)
    // VertexNormalWS -> MaskZ(B) -> Abs -> Power(SlopeSharpness) -> SlopePow
    // =================================================================
    auto* VNormal = NewObject<UMaterialExpressionVertexNormalWS>(Mat);
    AddExpr(VNormal, -300, -1500);

    auto* MaskZ = NewObject<UMaterialExpressionComponentMask>(Mat);
    MaskZ->R = false; MaskZ->G = false; MaskZ->B = true; MaskZ->A = false;
    MaskZ->Input.Connect(0, VNormal);
    AddExpr(MaskZ, 0, -1500);

    auto* AbsNode = NewObject<UMaterialExpressionAbs>(Mat);
    AbsNode->Input.Connect(0, MaskZ);
    AddExpr(AbsNode, 0, -1350);

    auto* SlopeParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
    SlopeParam->ParameterName = FName(TEXT("SlopeSharpness"));
    SlopeParam->DefaultValue = SlopeSharpness;
    AddExpr(SlopeParam, -300, -1650);

    auto* SlopePow = NewObject<UMaterialExpressionPower>(Mat);
    SlopePow->Base.Connect(0, AbsNode);
    SlopePow->Exponent.Connect(0, SlopeParam);
    AddExpr(SlopePow, 300, -1500);

    // =================================================================
    // SECTION 6: Grass Mask (6 nodes) - SEPARATE noise for grass distribution
    // GrassNoise(WorldPos, 0.0001) -> Power(2) -> Multiply(GrassAmount) -> GrassMask
    // SlopeGrassMask = Multiply(GrassMask, SlopePow) -- only on flat areas
    // =================================================================
    auto* GrassNoise = NewObject<UMaterialExpressionNoise>(Mat);
    GrassNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
    GrassNoise->Scale = 0.0001f;
    GrassNoise->Quality = 1;
    GrassNoise->Levels = 3;
    GrassNoise->OutputMin = 0.0f;
    GrassNoise->OutputMax = 1.0f;
    GrassNoise->bTurbulence = true;
    GrassNoise->bTiling = false;
    GrassNoise->LevelScale = 2.0f;
    GrassNoise->Position.Connect(0, WorldPos);
    AddExpr(GrassNoise, -300, 1600);

    auto* GrassPowConst = NewObject<UMaterialExpressionConstant>(Mat);
    GrassPowConst->R = 1.2f;  // Lower power = more responsive GrassAmount control
    AddExpr(GrassPowConst, -300, 1800);

    auto* GrassNoisePow = NewObject<UMaterialExpressionPower>(Mat);
    GrassNoisePow->Base.Connect(0, GrassNoise);
    GrassNoisePow->Exponent.Connect(0, GrassPowConst);
    AddExpr(GrassNoisePow, 0, 1700);

    auto* GrassParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
    GrassParam->ParameterName = FName(TEXT("GrassAmount"));
    GrassParam->DefaultValue = GrassAmount;
    AddExpr(GrassParam, 0, 1900);

    auto* GrassMask = NewObject<UMaterialExpressionMultiply>(Mat);
    GrassMask->A.Connect(0, GrassNoisePow);
    GrassMask->B.Connect(0, GrassParam);
    AddExpr(GrassMask, 300, 1700);

    // Slope-filtered grass: grass only on flat areas, not slopes
    auto* SlopeGrassMask = NewObject<UMaterialExpressionMultiply>(Mat);
    SlopeGrassMask->A.Connect(0, GrassMask);
    SlopeGrassMask->B.Connect(0, SlopePow);
    AddExpr(SlopeGrassMask, 500, 1700);

    // =================================================================
    // SECTION 7: Height-Based Blend Chains + Transition Noise
    // Uses texture luminance as pseudo-height for natural overlap,
    // plus high-frequency noise at blend boundaries.
    // =================================================================
    // HeightBlendStrength parameter (exposed in MI, used by blend section)
    auto* HeightBlendParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
    HeightBlendParam->ParameterName = FName(TEXT("HeightBlendStrength"));
    HeightBlendParam->DefaultValue = HeightBlendStrength;
    AddExpr(HeightBlendParam, 900, 2100);

    UMaterialExpression* FinalBC = nullptr;
    UMaterialExpression* BlendAlpha = SlopePow;  // default to simple slope

    if (RockLayer.Diffuse && MudLayer.Diffuse)
    {
        // --- UPGRADE 1: Height-based blending ---
        // Compute luminance of each layer as pseudo-height
        auto* LumWeights = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        LumWeights->Constant = FLinearColor(0.3f, 0.6f, 0.1f, 0.0f);
        AddExpr(LumWeights, -100, -800);

        auto* RockHeight = NewObject<UMaterialExpressionDotProduct>(Mat);
        RockHeight->A.Connect(0, RockLayer.Diffuse);
        RockHeight->B.Connect(0, LumWeights);
        AddExpr(RockHeight, 100, -900);

        auto* MudHeight = NewObject<UMaterialExpressionDotProduct>(Mat);
        MudHeight->A.Connect(0, MudLayer.Diffuse);
        MudHeight->B.Connect(0, LumWeights);
        AddExpr(MudHeight, 100, -700);

        // HeightDiff = (RockHeight - MudHeight) * HeightBlendStrength
        auto* HeightDiff = NewObject<UMaterialExpressionSubtract>(Mat);
        HeightDiff->A.Connect(0, RockHeight);
        HeightDiff->B.Connect(0, MudHeight);
        AddExpr(HeightDiff, 300, -800);

        auto* HeightMod = NewObject<UMaterialExpressionMultiply>(Mat);
        HeightMod->A.Connect(0, HeightDiff);
        HeightMod->B.Connect(0, HeightBlendParam);
        AddExpr(HeightMod, 500, -800);

        // AdjustedAlpha = Clamp(SlopePow + HeightDiff*Strength, 0, 1)
        auto* AlphaAdd = NewObject<UMaterialExpressionAdd>(Mat);
        AlphaAdd->A.Connect(0, SlopePow);
        AlphaAdd->B.Connect(0, HeightMod);
        AddExpr(AlphaAdd, 700, -800);

        auto* AlphaClamp = NewObject<UMaterialExpressionClamp>(Mat);
        AlphaClamp->Input.Connect(0, AlphaAdd);
        AddExpr(AlphaClamp, 900, -800);

        // --- UPGRADE 2: Noise at blend boundaries ---
        auto* TransNoisePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        TransNoisePosOffset->Constant = FLinearColor(5000.0f, 9000.0f, 0.0f, 0.0f);
        AddExpr(TransNoisePosOffset, 300, -600);

        auto* TransNoisePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        TransNoisePosAdd->A.Connect(0, WorldPos);
        TransNoisePosAdd->B.Connect(0, TransNoisePosOffset);
        AddExpr(TransNoisePosAdd, 500, -600);

        auto* TransNoise = NewObject<UMaterialExpressionNoise>(Mat);
        TransNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        TransNoise->Scale = 0.001f;
        TransNoise->Quality = 1;
        TransNoise->Levels = 3;
        TransNoise->OutputMin = -0.15f;
        TransNoise->OutputMax = 0.15f;
        TransNoise->bTurbulence = false;
        TransNoise->bTiling = false;
        TransNoise->LevelScale = 2.0f;
        TransNoise->Position.Connect(0, TransNoisePosAdd);
        AddExpr(TransNoise, 700, -600);

        // FinalAlpha = Clamp(AdjustedAlpha + TransNoise, 0, 1)
        auto* FinalAlphaAdd = NewObject<UMaterialExpressionAdd>(Mat);
        FinalAlphaAdd->A.Connect(0, AlphaClamp);
        FinalAlphaAdd->B.Connect(0, TransNoise);
        AddExpr(FinalAlphaAdd, 900, -600);

        auto* FinalAlphaClamp = NewObject<UMaterialExpressionClamp>(Mat);
        FinalAlphaClamp->Input.Connect(0, FinalAlphaAdd);
        AddExpr(FinalAlphaClamp, 1100, -700);

        BlendAlpha = FinalAlphaClamp;

        auto* SlopeBC = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        SlopeBC->A.Connect(0, RockLayer.Diffuse);
        SlopeBC->B.Connect(0, MudLayer.Diffuse);
        SlopeBC->Alpha.Connect(0, BlendAlpha);
        AddExpr(SlopeBC, 300, -200);
        FinalBC = SlopeBC;
    }
    else if (RockLayer.Diffuse) { FinalBC = RockLayer.Diffuse; }
    else if (MudLayer.Diffuse) { FinalBC = MudLayer.Diffuse; }

    if (GrassLayer.Diffuse && FinalBC)
    {
        auto* GrassBC = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        GrassBC->A.Connect(0, FinalBC);
        GrassBC->B.Connect(0, GrassLayer.Diffuse);
        GrassBC->Alpha.Connect(0, SlopeGrassMask);
        AddExpr(GrassBC, 600, -200);
        FinalBC = GrassBC;
    }

    // Normal blend chain -- uses same height-adjusted alpha for rock/mud normals
    UMaterialExpression* FinalN = nullptr;

    if (RockLayer.Normal && MudLayer.Normal)
    {
        auto* SlopeN = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        SlopeN->A.Connect(0, RockLayer.Normal);
        SlopeN->B.Connect(0, MudLayer.Normal);
        SlopeN->Alpha.Connect(0, BlendAlpha);
        AddExpr(SlopeN, 300, 600);
        FinalN = SlopeN;
    }
    else if (RockLayer.Normal) { FinalN = RockLayer.Normal; }
    else if (MudLayer.Normal) { FinalN = MudLayer.Normal; }

    if (GrassLayer.Normal && FinalN)
    {
        auto* GrassNLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        GrassNLerp->A.Connect(0, FinalN);
        GrassNLerp->B.Connect(0, GrassLayer.Normal);
        GrassNLerp->Alpha.Connect(0, SlopeGrassMask);
        AddExpr(GrassNLerp, 600, 600);
        FinalN = GrassNLerp;
    }

    // Roughness param (created early so puddle section can reference it)
    auto* RoughParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
    RoughParam->ParameterName = FName(TEXT("Roughness"));
    RoughParam->DefaultValue = RoughnessVal;
    AddExpr(RoughParam, 900, 200);
    UMaterialExpression* FinalRough = RoughParam;

    auto* PuddleHeightBiasParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
    PuddleHeightBiasParam->ParameterName = FName(TEXT("PuddleHeightBias"));
    PuddleHeightBiasParam->DefaultValue = PuddleHeightBias;
    AddExpr(PuddleHeightBiasParam, 900, 500);

    // =================================================================
    // SHARED: World-Z Height Bias for Puddles + Mud
    // Low-lying areas get more puddles and mud. Uses AbsoluteWorldPosition.Z
    // normalized to 0-1, inverted so low = high value.
    // =================================================================
    // Extract Z from WorldPos
    auto* MaskWorldZ = NewObject<UMaterialExpressionComponentMask>(Mat);
    MaskWorldZ->R = false; MaskWorldZ->G = false; MaskWorldZ->B = true; MaskWorldZ->A = false;
    MaskWorldZ->Input.Connect(0, WorldPos);
    AddExpr(MaskWorldZ, 700, 800);

    // Normalize Z to 0-1 range (landscape Z range ~3500 units, use /5000 + 0.5)
    auto* ZDivConst = NewObject<UMaterialExpressionConstant>(Mat);
    ZDivConst->R = 5000.0f;
    AddExpr(ZDivConst, 700, 950);

    auto* ZDiv = NewObject<UMaterialExpressionDivide>(Mat);
    ZDiv->A.Connect(0, MaskWorldZ);
    ZDiv->B.Connect(0, ZDivConst);
    AddExpr(ZDiv, 900, 850);

    auto* ZHalfConst = NewObject<UMaterialExpressionConstant>(Mat);
    ZHalfConst->R = 0.5f;
    AddExpr(ZHalfConst, 900, 1000);

    auto* ZNorm = NewObject<UMaterialExpressionAdd>(Mat);
    ZNorm->A.Connect(0, ZDiv);
    ZNorm->B.Connect(0, ZHalfConst);
    AddExpr(ZNorm, 1100, 900);

    // Invert: low areas = high value
    auto* ZInvert = NewObject<UMaterialExpressionOneMinus>(Mat);
    ZInvert->Input.Connect(0, ZNorm);
    AddExpr(ZInvert, 1300, 900);

    // Clamp to 0-1
    auto* ZClamp = NewObject<UMaterialExpressionClamp>(Mat);
    ZClamp->Input.Connect(0, ZInvert);
    AddExpr(ZClamp, 1500, 900);

    // Power for concentration in valleys: Power(LowAreaMask, PuddleHeightBias)
    auto* HeightBiasPow = NewObject<UMaterialExpressionPower>(Mat);
    HeightBiasPow->Base.Connect(0, ZClamp);
    HeightBiasPow->Exponent.Connect(0, PuddleHeightBiasParam);
    AddExpr(HeightBiasPow, 1700, 900);
    UMaterialExpression* LowAreaMask = HeightBiasPow;

    // =================================================================
    // COMMENT BOX 8: Mud/Dirt Overlay (Dark Brown)
    // Noise-driven sandy patches on flat areas. Uses T_Sand_D if provided.
    // =================================================================
    UTexture* TexMudDetail = LoadTex(MudDetailD);

    if (TexMudDetail && FinalBC)
    {
        AddComment(TEXT("10. Dirt Patches (concentrated in low areas)"), FLinearColor(0.5f, 0.35f, 0.1f), 1800, -1700, 1200, 700);

        // --- UPGRADE 6: Multi-octave mud noise ---
        // Zone noise: large clusters where mud CAN form
        auto* MudZonePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        MudZonePosOffset->Constant = FLinearColor(7000.0f, 3000.0f, 0.0f, 0.0f);
        AddExpr(MudZonePosOffset, 1850, -1600);

        auto* MudZonePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        MudZonePosAdd->A.Connect(0, WorldPos);
        MudZonePosAdd->B.Connect(0, MudZonePosOffset);
        AddExpr(MudZonePosAdd, 2050, -1600);

        auto* MudZoneNoise = NewObject<UMaterialExpressionNoise>(Mat);
        MudZoneNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        MudZoneNoise->Scale = 0.0001f;   // Large zone clusters
        MudZoneNoise->Quality = 1;
        MudZoneNoise->Levels = 3;
        MudZoneNoise->OutputMin = 0.0f;
        MudZoneNoise->OutputMax = 1.0f;
        MudZoneNoise->bTurbulence = true;
        MudZoneNoise->bTiling = false;
        MudZoneNoise->LevelScale = 2.0f;
        MudZoneNoise->Position.Connect(0, MudZonePosAdd);
        AddExpr(MudZoneNoise, 2250, -1600);

        // Shape noise: individual mud shapes within zones
        auto* MudShapePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        MudShapePosOffset->Constant = FLinearColor(20000.0f, 5000.0f, 0.0f, 0.0f);
        AddExpr(MudShapePosOffset, 1850, -1400);

        auto* MudShapePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        MudShapePosAdd->A.Connect(0, WorldPos);
        MudShapePosAdd->B.Connect(0, MudShapePosOffset);
        AddExpr(MudShapePosAdd, 2050, -1400);

        auto* MudShapeNoise = NewObject<UMaterialExpressionNoise>(Mat);
        MudShapeNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        MudShapeNoise->Scale = 0.0006f;  // Smaller individual shapes
        MudShapeNoise->Quality = 1;
        MudShapeNoise->Levels = 2;
        MudShapeNoise->OutputMin = 0.0f;
        MudShapeNoise->OutputMax = 1.0f;
        MudShapeNoise->bTurbulence = false;
        MudShapeNoise->bTiling = false;
        MudShapeNoise->LevelScale = 2.0f;
        MudShapeNoise->Position.Connect(0, MudShapePosAdd);
        AddExpr(MudShapeNoise, 2250, -1400);

        // Multiply zone * shape for concentration
        auto* MudCombined = NewObject<UMaterialExpressionMultiply>(Mat);
        MudCombined->A.Connect(0, MudZoneNoise);
        MudCombined->B.Connect(0, MudShapeNoise);
        AddExpr(MudCombined, 2450, -1500);

        // Sharpen edges
        auto* MudPowConst = NewObject<UMaterialExpressionConstant>(Mat);
        MudPowConst->R = 1.5f;
        AddExpr(MudPowConst, 2450, -1350);

        auto* MudMaskPow = NewObject<UMaterialExpressionPower>(Mat);
        MudMaskPow->Base.Connect(0, MudCombined);
        MudMaskPow->Exponent.Connect(0, MudPowConst);
        AddExpr(MudMaskPow, 2650, -1450);

        // MudAmount MI parameter
        auto* MudAmountParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
        MudAmountParam->ParameterName = FName(TEXT("MudAmount"));
        MudAmountParam->DefaultValue = MudAmount;
        AddExpr(MudAmountParam, 2450, -1200);

        // MudAlpha = MudMask * MudAmount * SlopePow * LowAreaMask
        auto* MudAmountMul = NewObject<UMaterialExpressionMultiply>(Mat);
        MudAmountMul->A.Connect(0, MudMaskPow);
        MudAmountMul->B.Connect(0, MudAmountParam);
        AddExpr(MudAmountMul, 2650, -1300);

        auto* MudSlopeMul = NewObject<UMaterialExpressionMultiply>(Mat);
        MudSlopeMul->A.Connect(0, MudAmountMul);
        MudSlopeMul->B.Connect(0, SlopePow);
        AddExpr(MudSlopeMul, 2850, -1350);

        auto* MudAlpha = NewObject<UMaterialExpressionMultiply>(Mat);
        MudAlpha->A.Connect(0, MudSlopeMul);
        MudAlpha->B.Connect(0, LowAreaMask);
        AddExpr(MudAlpha, 3050, -1350);

        // Sample mud detail texture
        auto* MudDetailSample = CreateTexSample(TexMudDetail, SAMPLERTYPE_Color, DistortedUV, 1850, -1200);

        // Blend mud into base color
        auto* MudBC = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        MudBC->A.Connect(0, FinalBC);
        MudBC->B.Connect(0, MudDetailSample);
        MudBC->Alpha.Connect(0, MudAlpha);
        AddExpr(MudBC, 3050, -1200);
        FinalBC = MudBC;
    }

    // =================================================================
    // COMMENT BOX 10B: Rubble Overlay (Tan/Beige)
    // Scattered rubble patches — lighter rocky debris on flat areas.
    // Multi-octave noise for concentrated patches.
    // =================================================================
    if (FinalBC)
    {
        AddComment(TEXT("12. Rubble Patches (scattered rocky debris)"), FLinearColor(0.7f, 0.6f, 0.3f), 3300, -1700, 1200, 700);

        // Rubble zone noise: where rubble clusters form
        auto* RubbleZonePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        RubbleZonePosOffset->Constant = FLinearColor(25000.0f, 14000.0f, 0.0f, 0.0f);
        AddExpr(RubbleZonePosOffset, 3350, -1600);

        auto* RubbleZonePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        RubbleZonePosAdd->A.Connect(0, WorldPos);
        RubbleZonePosAdd->B.Connect(0, RubbleZonePosOffset);
        AddExpr(RubbleZonePosAdd, 3550, -1600);

        auto* RubbleZoneNoise = NewObject<UMaterialExpressionNoise>(Mat);
        RubbleZoneNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        RubbleZoneNoise->Scale = 0.00012f;  // Large zone clusters
        RubbleZoneNoise->Quality = 1;
        RubbleZoneNoise->Levels = 3;
        RubbleZoneNoise->OutputMin = 0.0f;
        RubbleZoneNoise->OutputMax = 1.0f;
        RubbleZoneNoise->bTurbulence = true;
        RubbleZoneNoise->bTiling = false;
        RubbleZoneNoise->LevelScale = 2.0f;
        RubbleZoneNoise->Position.Connect(0, RubbleZonePosAdd);
        AddExpr(RubbleZoneNoise, 3750, -1600);

        // Shape noise: individual rubble patches
        auto* RubbleShapePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        RubbleShapePosOffset->Constant = FLinearColor(30000.0f, 18000.0f, 0.0f, 0.0f);
        AddExpr(RubbleShapePosOffset, 3350, -1400);

        auto* RubbleShapePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        RubbleShapePosAdd->A.Connect(0, WorldPos);
        RubbleShapePosAdd->B.Connect(0, RubbleShapePosOffset);
        AddExpr(RubbleShapePosAdd, 3550, -1400);

        auto* RubbleShapeNoise = NewObject<UMaterialExpressionNoise>(Mat);
        RubbleShapeNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        RubbleShapeNoise->Scale = 0.0008f;  // Smaller shapes
        RubbleShapeNoise->Quality = 1;
        RubbleShapeNoise->Levels = 2;
        RubbleShapeNoise->OutputMin = 0.0f;
        RubbleShapeNoise->OutputMax = 1.0f;
        RubbleShapeNoise->bTurbulence = false;
        RubbleShapeNoise->bTiling = false;
        RubbleShapeNoise->LevelScale = 2.0f;
        RubbleShapeNoise->Position.Connect(0, RubbleShapePosAdd);
        AddExpr(RubbleShapeNoise, 3750, -1400);

        // Multiply zone * shape for concentration
        auto* RubbleCombined = NewObject<UMaterialExpressionMultiply>(Mat);
        RubbleCombined->A.Connect(0, RubbleZoneNoise);
        RubbleCombined->B.Connect(0, RubbleShapeNoise);
        AddExpr(RubbleCombined, 3950, -1500);

        // Sharpen edges
        auto* RubblePowConst = NewObject<UMaterialExpressionConstant>(Mat);
        RubblePowConst->R = 1.5f;
        AddExpr(RubblePowConst, 3950, -1350);

        auto* RubbleMaskPow = NewObject<UMaterialExpressionPower>(Mat);
        RubbleMaskPow->Base.Connect(0, RubbleCombined);
        RubbleMaskPow->Exponent.Connect(0, RubblePowConst);
        AddExpr(RubbleMaskPow, 4150, -1450);

        // RubbleAmount MI parameter
        auto* RubbleAmountParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
        RubbleAmountParam->ParameterName = FName(TEXT("RubbleAmount"));
        RubbleAmountParam->DefaultValue = RubbleAmount;
        AddExpr(RubbleAmountParam, 3950, -1200);

        // RubbleAlpha = RubbleMask * RubbleAmount * SlopePow (flat areas)
        auto* RubbleAmountMul = NewObject<UMaterialExpressionMultiply>(Mat);
        RubbleAmountMul->A.Connect(0, RubbleMaskPow);
        RubbleAmountMul->B.Connect(0, RubbleAmountParam);
        AddExpr(RubbleAmountMul, 4150, -1300);

        auto* RubbleSlopeMul = NewObject<UMaterialExpressionMultiply>(Mat);
        RubbleSlopeMul->A.Connect(0, RubbleAmountMul);
        RubbleSlopeMul->B.Connect(0, SlopePow);
        AddExpr(RubbleSlopeMul, 4350, -1350);

        // Rubble color: light tan/beige rocky debris
        auto* RubbleColor = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        RubbleColor->Constant = FLinearColor(0.18f, 0.14f, 0.09f, 1.0f);
        AddExpr(RubbleColor, 4150, -1150);

        // Blend rubble into base color
        auto* RubbleBC = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        RubbleBC->A.Connect(0, FinalBC);
        RubbleBC->B.Connect(0, RubbleColor);
        RubbleBC->Alpha.Connect(0, RubbleSlopeMul);
        AddExpr(RubbleBC, 4550, -1250);
        FinalBC = RubbleBC;
    }

    // =================================================================
    // COMMENT BOX 10C: Stone Overlay (Dark Grey)
    // Scattered dark stone patches — small hard stones/pebbles.
    // Multi-octave noise, prefers slopes more than rubble.
    // =================================================================
    if (FinalBC)
    {
        AddComment(TEXT("13. Stone Patches (hard pebbles on slopes)"), FLinearColor(0.4f, 0.4f, 0.5f), 3300, -900, 1200, 700);

        // Stone zone noise
        auto* StoneZonePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        StoneZonePosOffset->Constant = FLinearColor(35000.0f, 22000.0f, 0.0f, 0.0f);
        AddExpr(StoneZonePosOffset, 3350, -800);

        auto* StoneZonePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        StoneZonePosAdd->A.Connect(0, WorldPos);
        StoneZonePosAdd->B.Connect(0, StoneZonePosOffset);
        AddExpr(StoneZonePosAdd, 3550, -800);

        auto* StoneZoneNoise = NewObject<UMaterialExpressionNoise>(Mat);
        StoneZoneNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        StoneZoneNoise->Scale = 0.00015f;  // Medium zone clusters
        StoneZoneNoise->Quality = 1;
        StoneZoneNoise->Levels = 3;
        StoneZoneNoise->OutputMin = 0.0f;
        StoneZoneNoise->OutputMax = 1.0f;
        StoneZoneNoise->bTurbulence = true;
        StoneZoneNoise->bTiling = false;
        StoneZoneNoise->LevelScale = 2.0f;
        StoneZoneNoise->Position.Connect(0, StoneZonePosAdd);
        AddExpr(StoneZoneNoise, 3750, -800);

        // Shape noise: individual stone patches (higher frequency = smaller)
        auto* StoneShapePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        StoneShapePosOffset->Constant = FLinearColor(40000.0f, 28000.0f, 0.0f, 0.0f);
        AddExpr(StoneShapePosOffset, 3350, -600);

        auto* StoneShapePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        StoneShapePosAdd->A.Connect(0, WorldPos);
        StoneShapePosAdd->B.Connect(0, StoneShapePosOffset);
        AddExpr(StoneShapePosAdd, 3550, -600);

        auto* StoneShapeNoise = NewObject<UMaterialExpressionNoise>(Mat);
        StoneShapeNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        StoneShapeNoise->Scale = 0.001f;  // Small individual stones
        StoneShapeNoise->Quality = 1;
        StoneShapeNoise->Levels = 2;
        StoneShapeNoise->OutputMin = 0.0f;
        StoneShapeNoise->OutputMax = 1.0f;
        StoneShapeNoise->bTurbulence = false;
        StoneShapeNoise->bTiling = false;
        StoneShapeNoise->LevelScale = 2.0f;
        StoneShapeNoise->Position.Connect(0, StoneShapePosAdd);
        AddExpr(StoneShapeNoise, 3750, -600);

        // Multiply zone * shape
        auto* StoneCombined = NewObject<UMaterialExpressionMultiply>(Mat);
        StoneCombined->A.Connect(0, StoneZoneNoise);
        StoneCombined->B.Connect(0, StoneShapeNoise);
        AddExpr(StoneCombined, 3950, -700);

        // Sharpen edges
        auto* StonePowConst = NewObject<UMaterialExpressionConstant>(Mat);
        StonePowConst->R = 2.0f;
        AddExpr(StonePowConst, 3950, -550);

        auto* StoneMaskPow = NewObject<UMaterialExpressionPower>(Mat);
        StoneMaskPow->Base.Connect(0, StoneCombined);
        StoneMaskPow->Exponent.Connect(0, StonePowConst);
        AddExpr(StoneMaskPow, 4150, -650);

        // StoneAmount MI parameter
        auto* StoneAmountParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
        StoneAmountParam->ParameterName = FName(TEXT("StoneAmount"));
        StoneAmountParam->DefaultValue = StoneAmount;
        AddExpr(StoneAmountParam, 3950, -400);

        // StoneAlpha = StoneMask * StoneAmount * OneMinus(SlopePow) -- prefers slopes
        auto* InvSlopePow = NewObject<UMaterialExpressionOneMinus>(Mat);
        InvSlopePow->Input.Connect(0, SlopePow);
        AddExpr(InvSlopePow, 4150, -400);

        auto* StoneAmountMul = NewObject<UMaterialExpressionMultiply>(Mat);
        StoneAmountMul->A.Connect(0, StoneMaskPow);
        StoneAmountMul->B.Connect(0, StoneAmountParam);
        AddExpr(StoneAmountMul, 4350, -550);

        auto* StoneSlopeMul = NewObject<UMaterialExpressionMultiply>(Mat);
        StoneSlopeMul->A.Connect(0, StoneAmountMul);
        StoneSlopeMul->B.Connect(0, InvSlopePow);
        AddExpr(StoneSlopeMul, 4550, -500);

        // Stone color: dark grey (hard stone/pebble look)
        auto* StoneColor = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        StoneColor->Constant = FLinearColor(0.06f, 0.055f, 0.05f, 1.0f);
        AddExpr(StoneColor, 4350, -350);

        // Blend stone into base color
        auto* StoneBC = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        StoneBC->A.Connect(0, FinalBC);
        StoneBC->B.Connect(0, StoneColor);
        StoneBC->Alpha.Connect(0, StoneSlopeMul);
        AddExpr(StoneBC, 4750, -450);
        FinalBC = StoneBC;

        // Stone also lowers roughness slightly (smooth stone surface)
        auto* StoneRoughConst = NewObject<UMaterialExpressionConstant>(Mat);
        StoneRoughConst->R = 0.55f;
        AddExpr(StoneRoughConst, 4550, -300);

        auto* StoneRough = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        StoneRough->A.Connect(0, FinalRough);
        StoneRough->B.Connect(0, StoneRoughConst);
        StoneRough->Alpha.Connect(0, StoneSlopeMul);
        AddExpr(StoneRough, 4750, -300);
        FinalRough = StoneRough;
    }

    // =================================================================
    // COMMENT BOX 11: Puddle Overlay (Blue)
    // Procedural dark puddles on flat areas. No textures needed.
    // Dark color, very low roughness (wet/shiny), flat normal.
    // =================================================================
    if (FinalBC)
    {
        AddComment(TEXT("14. Puddles & Wet Areas (valleys, with wet edge halo)"), FLinearColor(0.2f, 0.4f, 0.8f), 1800, -800, 1400, 1200);

        // --- UPGRADE 3: Multi-octave puddle mask ---
        // Zone noise: large puddle clusters
        auto* PuddleZonePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        PuddleZonePosOffset->Constant = FLinearColor(11000.0f, 8000.0f, 0.0f, 0.0f);
        AddExpr(PuddleZonePosOffset, 1850, -700);

        auto* PuddleZonePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        PuddleZonePosAdd->A.Connect(0, WorldPos);
        PuddleZonePosAdd->B.Connect(0, PuddleZonePosOffset);
        AddExpr(PuddleZonePosAdd, 2050, -700);

        auto* PuddleZoneNoise = NewObject<UMaterialExpressionNoise>(Mat);
        PuddleZoneNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        PuddleZoneNoise->Scale = 0.00008f;  // Very large zones
        PuddleZoneNoise->Quality = 1;
        PuddleZoneNoise->Levels = 3;
        PuddleZoneNoise->OutputMin = 0.0f;
        PuddleZoneNoise->OutputMax = 1.0f;
        PuddleZoneNoise->bTurbulence = false;
        PuddleZoneNoise->bTiling = false;
        PuddleZoneNoise->LevelScale = 2.0f;
        PuddleZoneNoise->Position.Connect(0, PuddleZonePosAdd);
        AddExpr(PuddleZoneNoise, 2250, -700);

        // Shape noise: individual puddle shapes
        auto* PuddleShapePosOffset = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        PuddleShapePosOffset->Constant = FLinearColor(15000.0f, 12000.0f, 0.0f, 0.0f);
        AddExpr(PuddleShapePosOffset, 1850, -500);

        auto* PuddleShapePosAdd = NewObject<UMaterialExpressionAdd>(Mat);
        PuddleShapePosAdd->A.Connect(0, WorldPos);
        PuddleShapePosAdd->B.Connect(0, PuddleShapePosOffset);
        AddExpr(PuddleShapePosAdd, 2050, -500);

        auto* PuddleShapeNoise = NewObject<UMaterialExpressionNoise>(Mat);
        PuddleShapeNoise->NoiseFunction = NOISEFUNCTION_GradientALU;
        PuddleShapeNoise->Scale = 0.0005f;  // Medium shapes
        PuddleShapeNoise->Quality = 1;
        PuddleShapeNoise->Levels = 2;
        PuddleShapeNoise->OutputMin = 0.0f;
        PuddleShapeNoise->OutputMax = 1.0f;
        PuddleShapeNoise->bTurbulence = false;
        PuddleShapeNoise->bTiling = false;
        PuddleShapeNoise->LevelScale = 2.0f;
        PuddleShapeNoise->Position.Connect(0, PuddleShapePosAdd);
        AddExpr(PuddleShapeNoise, 2250, -500);

        // Multiply zone * shape for concentrated patches
        auto* PuddleCombined = NewObject<UMaterialExpressionMultiply>(Mat);
        PuddleCombined->A.Connect(0, PuddleZoneNoise);
        PuddleCombined->B.Connect(0, PuddleShapeNoise);
        AddExpr(PuddleCombined, 2450, -600);

        // Sharpen edges: Power(2.0)
        auto* PuddlePowConst = NewObject<UMaterialExpressionConstant>(Mat);
        PuddlePowConst->R = 2.0f;
        AddExpr(PuddlePowConst, 2450, -450);

        auto* PuddleMaskPow = NewObject<UMaterialExpressionPower>(Mat);
        PuddleMaskPow->Base.Connect(0, PuddleCombined);
        PuddleMaskPow->Exponent.Connect(0, PuddlePowConst);
        AddExpr(PuddleMaskPow, 2650, -550);

        // PuddleAmount MI parameter
        auto* PuddleAmountParam = NewObject<UMaterialExpressionScalarParameter>(Mat);
        PuddleAmountParam->ParameterName = FName(TEXT("PuddleAmount"));
        PuddleAmountParam->DefaultValue = PuddleAmount;
        AddExpr(PuddleAmountParam, 2450, -300);

        // --- UPGRADE 4: World-Z height bias ---
        // PuddleAlpha = PuddleMask * PuddleAmount * SlopePow * LowAreaMask
        auto* PuddleAmountMul = NewObject<UMaterialExpressionMultiply>(Mat);
        PuddleAmountMul->A.Connect(0, PuddleMaskPow);
        PuddleAmountMul->B.Connect(0, PuddleAmountParam);
        AddExpr(PuddleAmountMul, 2650, -400);

        auto* PuddleSlopeMul = NewObject<UMaterialExpressionMultiply>(Mat);
        PuddleSlopeMul->A.Connect(0, PuddleAmountMul);
        PuddleSlopeMul->B.Connect(0, SlopePow);
        AddExpr(PuddleSlopeMul, 2850, -450);

        auto* PuddleAlpha = NewObject<UMaterialExpressionMultiply>(Mat);
        PuddleAlpha->A.Connect(0, PuddleSlopeMul);
        PuddleAlpha->B.Connect(0, LowAreaMask);
        AddExpr(PuddleAlpha, 3050, -450);

        // Dark puddle color (very dark brown, like wet earth)
        auto* PuddleColor = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        PuddleColor->Constant = FLinearColor(0.02f, 0.015f, 0.01f, 1.0f);
        AddExpr(PuddleColor, 2850, -250);

        // Blend puddle color into base color
        auto* PuddleBC = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        PuddleBC->A.Connect(0, FinalBC);
        PuddleBC->B.Connect(0, PuddleColor);
        PuddleBC->Alpha.Connect(0, PuddleAlpha);
        AddExpr(PuddleBC, 3050, -250);

        // --- UPGRADE 5: Wet edge darkening ---
        // WetEdgeAlpha = Clamp(PuddleAlpha * 3.0, 0, 1)
        auto* WetExpandConst = NewObject<UMaterialExpressionConstant>(Mat);
        WetExpandConst->R = 3.0f;
        AddExpr(WetExpandConst, 2850, -100);

        auto* WetEdgeMul = NewObject<UMaterialExpressionMultiply>(Mat);
        WetEdgeMul->A.Connect(0, PuddleAlpha);
        WetEdgeMul->B.Connect(0, WetExpandConst);
        AddExpr(WetEdgeMul, 3050, -100);

        auto* WetEdgeAlpha = NewObject<UMaterialExpressionClamp>(Mat);
        WetEdgeAlpha->Input.Connect(0, WetEdgeMul);
        AddExpr(WetEdgeAlpha, 3250, -100);

        // Darken base color in wet areas: WetDarken = Lerp(1.0, 0.6, WetEdgeAlpha)
        auto* DryBright = NewObject<UMaterialExpressionConstant>(Mat);
        DryBright->R = 1.0f;
        AddExpr(DryBright, 3050, 50);

        auto* WetBright = NewObject<UMaterialExpressionConstant>(Mat);
        WetBright->R = 0.6f;
        AddExpr(WetBright, 3050, 150);

        auto* WetDarkenLerp = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        WetDarkenLerp->A.Connect(0, DryBright);
        WetDarkenLerp->B.Connect(0, WetBright);
        WetDarkenLerp->Alpha.Connect(0, WetEdgeAlpha);
        AddExpr(WetDarkenLerp, 3250, 50);

        // Apply darkening: FinalBC = PuddleBC * WetDarken
        auto* DarkenedBC = NewObject<UMaterialExpressionMultiply>(Mat);
        DarkenedBC->A.Connect(0, PuddleBC);
        DarkenedBC->B.Connect(0, WetDarkenLerp);
        AddExpr(DarkenedBC, 3450, -100);
        FinalBC = DarkenedBC;

        // Wet roughness: dry(Roughness) -> wet edge(0.4) -> puddle center(0.05)
        auto* WetEdgeRoughConst = NewObject<UMaterialExpressionConstant>(Mat);
        WetEdgeRoughConst->R = 0.4f;
        AddExpr(WetEdgeRoughConst, 3050, 250);

        auto* WetEdgeRough = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        WetEdgeRough->A.Connect(0, RoughParam);
        WetEdgeRough->B.Connect(0, WetEdgeRoughConst);
        WetEdgeRough->Alpha.Connect(0, WetEdgeAlpha);
        AddExpr(WetEdgeRough, 3250, 250);

        auto* PuddleCenterRoughConst = NewObject<UMaterialExpressionConstant>(Mat);
        PuddleCenterRoughConst->R = 0.05f;
        AddExpr(PuddleCenterRoughConst, 3050, 350);

        auto* PuddleRough = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
        PuddleRough->A.Connect(0, WetEdgeRough);
        PuddleRough->B.Connect(0, PuddleCenterRoughConst);
        PuddleRough->Alpha.Connect(0, PuddleAlpha);
        AddExpr(PuddleRough, 3250, 350);
        FinalRough = PuddleRough;

        // Flat normal for puddles (smooth water surface)
        auto* FlatNormal = NewObject<UMaterialExpressionConstant3Vector>(Mat);
        FlatNormal->Constant = FLinearColor(0.0f, 0.0f, 1.0f, 0.0f);
        AddExpr(FlatNormal, 3050, 450);

        if (FinalN)
        {
            auto* PuddleN = NewObject<UMaterialExpressionLinearInterpolate>(Mat);
            PuddleN->A.Connect(0, FinalN);
            PuddleN->B.Connect(0, FlatNormal);
            PuddleN->Alpha.Connect(0, PuddleAlpha);
            AddExpr(PuddleN, 3250, 450);
            FinalN = PuddleN;
        }
    }

    // =================================================================
    // SECTION 8: Connect All Material Outputs
    // =================================================================
    if (FinalBC) { Mat->GetEditorOnlyData()->BaseColor.Connect(0, FinalBC); }
    if (FinalN)  { Mat->GetEditorOnlyData()->Normal.Connect(0, FinalN); }
    Mat->GetEditorOnlyData()->Roughness.Connect(0, FinalRough);

    auto* MetalConst = NewObject<UMaterialExpressionConstant>(Mat);
    MetalConst->R = 0.0f;
    AddExpr(MetalConst, 900, 400);
    Mat->GetEditorOnlyData()->Metallic.Connect(0, MetalConst);

    // =========================================================
    // FINALIZE
    // =========================================================
    Mat->PostEditChange();
    Package->MarkPackageDirty();
    IAssetRegistry::Get()->AssetCreated(Mat);

    // Save the package to disk immediately
    FString PackageFilename;
    if (FPackageName::TryConvertLongPackageNameToFilename(FullPath, PackageFilename, FPackageName::GetAssetPackageExtension()))
    {
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Standalone;
        UPackage::SavePackage(Package, Mat, *PackageFilename, SaveArgs);
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), MaterialName);
    Result->SetStringField(TEXT("path"), FullPath);
    Result->SetNumberField(TEXT("expression_count"), Mat->GetExpressionCollection().Expressions.Num());
    Result->SetNumberField(TEXT("comment_count"), 14);
    Result->SetStringField(TEXT("message"), TEXT("Landscape material v9: height-based layer blend, transition noise, multi-octave puddle+mud+rubble+stone, World-Z height bias, wet edge darkening, distance tiling fade, UV distortion + rotation dissolve, 13 samplers, 12 exposed params"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleImportMesh(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    // Check if source file exists
    if (!FPaths::FileExists(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file not found: %s"), *SourcePath));
    }

    // Get optional parameters
    FString AssetName;
    if (!Params->TryGetStringField(TEXT("asset_name"), AssetName))
    {
        AssetName = FPaths::GetBaseFilename(SourcePath);
    }

    FString DestinationPath = TEXT("/Game/Meshes/");
    Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

    if (!DestinationPath.EndsWith(TEXT("/")))
    {
        DestinationPath += TEXT("/");
    }

    bool bImportMaterials = false;
    Params->TryGetBoolField(TEXT("import_materials"), bImportMaterials);

    bool bImportTextures = false;
    Params->TryGetBoolField(TEXT("import_textures"), bImportTextures);

    bool bGenerateCollision = true;
    Params->TryGetBoolField(TEXT("generate_collision"), bGenerateCollision);

    bool bEnableNanite = true;
    Params->TryGetBoolField(TEXT("enable_nanite"), bEnableNanite);

    bool bCombineMeshes = true;
    Params->TryGetBoolField(TEXT("combine_meshes"), bCombineMeshes);

    // Create the import task
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->AddToRoot();
    ImportTask->Filename = SourcePath;
    ImportTask->DestinationPath = DestinationPath;
    ImportTask->DestinationName = AssetName;
    ImportTask->bReplaceExisting = true;
    ImportTask->bAutomated = true;
    ImportTask->bSave = false;

    // Configure FBX import settings
    UFbxImportUI* FbxUI = NewObject<UFbxImportUI>();
    FbxUI->bImportMesh = true;
    FbxUI->bImportAnimations = false;
    FbxUI->bImportMaterials = bImportMaterials;
    FbxUI->bImportTextures = bImportTextures;
    FbxUI->bOverrideFullName = true;
    FbxUI->MeshTypeToImport = FBXIT_StaticMesh;

    // Configure static mesh import data
    FbxUI->StaticMeshImportData->bAutoGenerateCollision = bGenerateCollision;
    FbxUI->StaticMeshImportData->bCombineMeshes = bCombineMeshes;
    FbxUI->StaticMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    FbxUI->StaticMeshImportData->bComputeWeightedNormals = true;

    ImportTask->Options = FbxUI;

    // Execute import
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UAssetImportTask*> Tasks;
    Tasks.Add(ImportTask);
    AssetToolsModule.Get().ImportAssetTasks(Tasks);

    // Check results
    TArray<UObject*> ImportedObjects = ImportTask->GetObjects();

    ImportTask->RemoveFromRoot();

    if (ImportedObjects.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to import mesh from: %s"), *SourcePath));
    }

    UObject* ImportedObject = ImportedObjects[0];
    UStaticMesh* StaticMesh = Cast<UStaticMesh>(ImportedObject);

    // Enable Nanite if requested
    if (StaticMesh && bEnableNanite)
    {
        FMeshNaniteSettings NaniteSettings = StaticMesh->GetNaniteSettings();
        NaniteSettings.bEnabled = true;
        StaticMesh->SetNaniteSettings(NaniteSettings);
        StaticMesh->PostEditChange();
    }

    // CRITICAL: Save the imported mesh package to disk immediately.
    // Prevents unsaved packages from accumulating in memory and
    // causing GC pressure that can corrupt landscape streaming proxies.
    if (ImportedObject)
    {
        UPackage* MeshPackage = ImportedObject->GetOutermost();
        if (MeshPackage)
        {
            FString MeshPackagePath = DestinationPath + AssetName;
            FString MeshPackageFilename = FPackageName::LongPackageNameToFilename(MeshPackagePath, FPackageName::GetAssetPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            UPackage::SavePackage(MeshPackage, ImportedObject, *MeshPackageFilename, SaveArgs);
        }
    }

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), AssetName);
    Result->SetStringField(TEXT("path"), DestinationPath + AssetName);
    Result->SetStringField(TEXT("source"), SourcePath);
    Result->SetStringField(TEXT("class"), ImportedObject->GetClass()->GetName());

    if (StaticMesh)
    {
        // Get mesh stats
        if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
        {
            const FStaticMeshLODResources& LOD0 = StaticMesh->GetRenderData()->LODResources[0];
            Result->SetNumberField(TEXT("vertex_count"), LOD0.GetNumVertices());
            Result->SetNumberField(TEXT("triangle_count"), LOD0.GetNumTriangles());
        }

        // Get material slots
        TArray<TSharedPtr<FJsonValue>> MaterialSlots;
        for (const FStaticMaterial& Mat : StaticMesh->GetStaticMaterials())
        {
            TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
            SlotObj->SetStringField(TEXT("name"), Mat.MaterialSlotName.ToString());
            SlotObj->SetStringField(TEXT("material"), Mat.MaterialInterface ? Mat.MaterialInterface->GetPathName() : TEXT("None"));
            MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotObj));
        }
        Result->SetArrayField(TEXT("material_slots"), MaterialSlots);

        // Bounding box
        FBox BoundingBox = StaticMesh->GetBoundingBox();
        TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();
        BoundsObj->SetNumberField(TEXT("min_x"), BoundingBox.Min.X);
        BoundsObj->SetNumberField(TEXT("min_y"), BoundingBox.Min.Y);
        BoundsObj->SetNumberField(TEXT("min_z"), BoundingBox.Min.Z);
        BoundsObj->SetNumberField(TEXT("max_x"), BoundingBox.Max.X);
        BoundsObj->SetNumberField(TEXT("max_y"), BoundingBox.Max.Y);
        BoundsObj->SetNumberField(TEXT("max_z"), BoundingBox.Max.Z);
        Result->SetObjectField(TEXT("bounds"), BoundsObj);

        Result->SetBoolField(TEXT("nanite_enabled"), StaticMesh->IsNaniteEnabled());
    }

    Result->SetStringField(TEXT("message"), TEXT("Mesh imported successfully"));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleImportSkeletalMesh(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    if (!FPaths::FileExists(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file not found: %s"), *SourcePath));
    }

    // Check file size to detect corrupt FBX files (known issue: some are 94 bytes with JSON text)
    int64 FileSize = IFileManager::Get().FileSize(*SourcePath);
    if (FileSize < 1024)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file too small (%lld bytes), likely corrupt: %s"), FileSize, *SourcePath));
    }

    // Get optional parameters
    FString AssetName;
    if (!Params->TryGetStringField(TEXT("asset_name"), AssetName))
    {
        AssetName = FPaths::GetBaseFilename(SourcePath);
    }

    FString DestinationPath = TEXT("/Game/Characters/");
    Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

    if (!DestinationPath.EndsWith(TEXT("/")))
    {
        DestinationPath += TEXT("/");
    }

    bool bImportAnimations = false;
    Params->TryGetBoolField(TEXT("import_animations"), bImportAnimations);

    bool bCreatePhysicsAsset = true;
    Params->TryGetBoolField(TEXT("create_physics_asset"), bCreatePhysicsAsset);

    bool bImportMorphTargets = true;
    Params->TryGetBoolField(TEXT("import_morph_targets"), bImportMorphTargets);

    bool bImportMaterials = false;
    Params->TryGetBoolField(TEXT("import_materials"), bImportMaterials);

    bool bImportTextures = false;
    Params->TryGetBoolField(TEXT("import_textures"), bImportTextures);

    // Optional: reuse existing skeleton
    FString SkeletonPath;
    Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath);

    // Create the import task
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->AddToRoot();
    ImportTask->Filename = SourcePath;
    ImportTask->DestinationPath = DestinationPath;
    ImportTask->DestinationName = AssetName;
    ImportTask->bReplaceExisting = true;
    ImportTask->bAutomated = true;
    ImportTask->bSave = false;

    // Configure FBX import settings for SKELETAL mesh
    UFbxImportUI* FbxUI = NewObject<UFbxImportUI>();
    FbxUI->bImportMesh = true;
    FbxUI->bImportAsSkeletal = true;
    FbxUI->MeshTypeToImport = FBXIT_SkeletalMesh;
    FbxUI->bImportAnimations = bImportAnimations;
    FbxUI->bImportMaterials = bImportMaterials;
    FbxUI->bImportTextures = bImportTextures;
    FbxUI->bOverrideFullName = true;
    FbxUI->bCreatePhysicsAsset = bCreatePhysicsAsset;

    // Configure skeletal mesh import data
    FbxUI->SkeletalMeshImportData->bImportMorphTargets = bImportMorphTargets;
    FbxUI->SkeletalMeshImportData->NormalImportMethod = FBXNIM_ImportNormalsAndTangents;
    FbxUI->SkeletalMeshImportData->bComputeWeightedNormals = true;

    // Reuse existing skeleton if specified
    if (!SkeletonPath.IsEmpty())
    {
        USkeleton* ExistingSkeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath);
        if (ExistingSkeleton)
        {
            FbxUI->Skeleton = ExistingSkeleton;
            UE_LOG(LogTemp, Log, TEXT("import_skeletal_mesh: Reusing existing skeleton: %s"), *SkeletonPath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("import_skeletal_mesh: Could not load skeleton at '%s', will create new"), *SkeletonPath);
        }
    }

    ImportTask->Options = FbxUI;

    // Execute import
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UAssetImportTask*> Tasks;
    Tasks.Add(ImportTask);
    AssetToolsModule.Get().ImportAssetTasks(Tasks);

    // Collect all imported objects
    TArray<UObject*> ImportedObjects = ImportTask->GetObjects();
    ImportTask->RemoveFromRoot();

    if (ImportedObjects.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to import skeletal mesh from: %s"), *SourcePath));
    }

    // Find the skeletal mesh and skeleton among imported objects
    USkeletalMesh* SkeletalMesh = nullptr;
    USkeleton* Skeleton = nullptr;
    TArray<UAnimSequence*> ImportedAnims;

    for (UObject* Obj : ImportedObjects)
    {
        if (USkeletalMesh* SK = Cast<USkeletalMesh>(Obj))
        {
            SkeletalMesh = SK;
            Skeleton = SK->GetSkeleton();
        }
        else if (UAnimSequence* Anim = Cast<UAnimSequence>(Obj))
        {
            ImportedAnims.Add(Anim);
        }
    }

    // Save all imported packages immediately to prevent memory accumulation
    for (UObject* Obj : ImportedObjects)
    {
        if (Obj)
        {
            UPackage* ObjPackage = Obj->GetOutermost();
            if (ObjPackage)
            {
                FString ObjPackagePath = ObjPackage->GetName();
                FString ObjPackageFilename = FPackageName::LongPackageNameToFilename(ObjPackagePath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(ObjPackage, Obj, *ObjPackageFilename, SaveArgs);
            }
        }
    }

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), AssetName);
    Result->SetStringField(TEXT("path"), DestinationPath + AssetName);
    Result->SetStringField(TEXT("source"), SourcePath);
    Result->SetNumberField(TEXT("imported_objects_count"), ImportedObjects.Num());

    if (SkeletalMesh)
    {
        Result->SetStringField(TEXT("class"), TEXT("SkeletalMesh"));
        Result->SetStringField(TEXT("skeletal_mesh_path"), SkeletalMesh->GetPathName());

        if (Skeleton)
        {
            Result->SetStringField(TEXT("skeleton_path"), Skeleton->GetPathName());
            Result->SetNumberField(TEXT("bone_count"), Skeleton->GetReferenceSkeleton().GetNum());

            // List bone names
            TArray<TSharedPtr<FJsonValue>> BoneNames;
            const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
            for (int32 i = 0; i < FMath::Min(RefSkeleton.GetNum(), 50); i++) // Cap at 50 to avoid huge responses
            {
                BoneNames.Add(MakeShared<FJsonValueString>(RefSkeleton.GetBoneName(i).ToString()));
            }
            Result->SetArrayField(TEXT("bone_names"), BoneNames);
        }

        // Material slots
        TArray<TSharedPtr<FJsonValue>> MaterialSlots;
        for (const FSkeletalMaterial& Mat : SkeletalMesh->GetMaterials())
        {
            TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
            SlotObj->SetStringField(TEXT("name"), Mat.MaterialSlotName.ToString());
            SlotObj->SetStringField(TEXT("material"), Mat.MaterialInterface ? Mat.MaterialInterface->GetPathName() : TEXT("None"));
            MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotObj));
        }
        Result->SetArrayField(TEXT("material_slots"), MaterialSlots);

        // Morph targets
        TArray<TSharedPtr<FJsonValue>> MorphTargetNames;
        for (UMorphTarget* MorphTarget : SkeletalMesh->GetMorphTargets())
        {
            if (MorphTarget)
            {
                MorphTargetNames.Add(MakeShared<FJsonValueString>(MorphTarget->GetName()));
            }
        }
        Result->SetArrayField(TEXT("morph_targets"), MorphTargetNames);
    }
    else
    {
        Result->SetStringField(TEXT("class"), ImportedObjects[0]->GetClass()->GetName());
    }

    // Include imported animation info
    if (ImportedAnims.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> AnimArray;
        for (UAnimSequence* Anim : ImportedAnims)
        {
            TSharedPtr<FJsonObject> AnimObj = MakeShared<FJsonObject>();
            AnimObj->SetStringField(TEXT("name"), Anim->GetName());
            AnimObj->SetStringField(TEXT("path"), Anim->GetPathName());
            AnimObj->SetNumberField(TEXT("duration"), Anim->GetPlayLength());
            AnimObj->SetNumberField(TEXT("num_frames"), Anim->GetNumberOfSampledKeys());
            AnimArray.Add(MakeShared<FJsonValueObject>(AnimObj));
        }
        Result->SetArrayField(TEXT("imported_animations"), AnimArray);
    }

    Result->SetStringField(TEXT("message"), TEXT("Skeletal mesh imported successfully"));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleImportAnimation(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    if (!FPaths::FileExists(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file not found: %s"), *SourcePath));
    }

    // Skeleton is REQUIRED for animation-only import
    FString SkeletonPath;
    if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Missing 'skeleton_path' parameter. Animation import requires an existing skeleton. "
                 "Import a skeletal mesh first to create one."));
    }

    USkeleton* TargetSkeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath);
    if (!TargetSkeleton)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not load skeleton at: %s"), *SkeletonPath));
    }

    // Get optional parameters
    FString AssetName;
    if (!Params->TryGetStringField(TEXT("animation_name"), AssetName))
    {
        AssetName = FPaths::GetBaseFilename(SourcePath);
    }

    FString DestinationPath = TEXT("/Game/Characters/Animations/");
    Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

    if (!DestinationPath.EndsWith(TEXT("/")))
    {
        DestinationPath += TEXT("/");
    }

    // Create the import task
    UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
    ImportTask->AddToRoot();
    ImportTask->Filename = SourcePath;
    ImportTask->DestinationPath = DestinationPath;
    ImportTask->DestinationName = AssetName;
    ImportTask->bReplaceExisting = true;
    ImportTask->bAutomated = true;
    ImportTask->bSave = false;

    // Configure FBX import settings for ANIMATION-ONLY import
    UFbxImportUI* FbxUI = NewObject<UFbxImportUI>();
    FbxUI->bImportMesh = false;
    FbxUI->bImportAnimations = true;
    FbxUI->MeshTypeToImport = FBXIT_Animation;
    FbxUI->bImportMaterials = false;
    FbxUI->bImportTextures = false;
    FbxUI->bOverrideFullName = true;
    FbxUI->Skeleton = TargetSkeleton;

    ImportTask->Options = FbxUI;

    // Execute import
    FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
    TArray<UAssetImportTask*> Tasks;
    Tasks.Add(ImportTask);
    AssetToolsModule.Get().ImportAssetTasks(Tasks);

    // Collect results
    TArray<UObject*> ImportedObjects = ImportTask->GetObjects();
    ImportTask->RemoveFromRoot();

    if (ImportedObjects.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to import animation from: %s. Ensure the FBX contains animation data compatible with the target skeleton."), *SourcePath));
    }

    // Save all imported animation packages
    for (UObject* Obj : ImportedObjects)
    {
        if (Obj)
        {
            UPackage* ObjPackage = Obj->GetOutermost();
            if (ObjPackage)
            {
                FString ObjPackagePath = ObjPackage->GetName();
                FString ObjPackageFilename = FPackageName::LongPackageNameToFilename(ObjPackagePath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(ObjPackage, Obj, *ObjPackageFilename, SaveArgs);
            }
        }
    }

    // Build result - iterate all imported objects (may contain multiple AnimSequences)
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("source"), SourcePath);
    Result->SetStringField(TEXT("skeleton_path"), SkeletonPath);
    Result->SetNumberField(TEXT("imported_count"), ImportedObjects.Num());

    TArray<TSharedPtr<FJsonValue>> AnimArray;
    for (UObject* Obj : ImportedObjects)
    {
        UAnimSequence* Anim = Cast<UAnimSequence>(Obj);
        if (Anim)
        {
            TSharedPtr<FJsonObject> AnimObj = MakeShared<FJsonObject>();
            AnimObj->SetStringField(TEXT("name"), Anim->GetName());
            AnimObj->SetStringField(TEXT("path"), Anim->GetPathName());
            AnimObj->SetNumberField(TEXT("duration_seconds"), Anim->GetPlayLength());
            AnimObj->SetNumberField(TEXT("num_frames"), Anim->GetNumberOfSampledKeys());
            AnimObj->SetStringField(TEXT("rate_scale"), FString::SanitizeFloat(Anim->RateScale));
            AnimArray.Add(MakeShared<FJsonValueObject>(AnimObj));
        }
    }
    Result->SetArrayField(TEXT("animations"), AnimArray);

    if (AnimArray.Num() == 0)
    {
        Result->SetStringField(TEXT("warning"), TEXT("Import succeeded but no AnimSequence assets were created. The FBX may contain only mesh data."));
    }

    Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Imported %d animation(s) successfully"), AnimArray.Num()));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleListAssets(const TSharedPtr<FJsonObject>& Params)
{
    FString Path = TEXT("/Game/");
    Params->TryGetStringField(TEXT("path"), Path);

    FString AssetType;
    Params->TryGetStringField(TEXT("asset_type"), AssetType);

    bool bRecursive = true;
    Params->TryGetBoolField(TEXT("recursive"), bRecursive);

    // Build asset filter
    FARFilter Filter;
    Filter.PackagePaths.Add(FName(*Path));
    Filter.bRecursivePaths = bRecursive;

    // Add class filter if specified
    if (!AssetType.IsEmpty())
    {
        FTopLevelAssetPath ClassPath(TEXT("/Script/Engine"), *AssetType);
        Filter.ClassPaths.Add(ClassPath);
    }

    // Query asset registry
    IAssetRegistry& AssetRegistry = *IAssetRegistry::Get();
    TArray<FAssetData> AssetDataList;
    AssetRegistry.GetAssets(Filter, AssetDataList);

    // Build result
    TArray<TSharedPtr<FJsonValue>> AssetsArray;
    for (const FAssetData& AssetData : AssetDataList)
    {
        TSharedPtr<FJsonObject> AssetObj = MakeShared<FJsonObject>();
        AssetObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        AssetObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        AssetObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
        AssetObj->SetStringField(TEXT("package_path"), AssetData.PackagePath.ToString());
        AssetsArray.Add(MakeShared<FJsonValueObject>(AssetObj));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("count"), AssetDataList.Num());
    Result->SetArrayField(TEXT("assets"), AssetsArray);

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDoesAssetExist(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    bool bExists = UEditorAssetLibrary::DoesAssetExist(AssetPath);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetBoolField(TEXT("exists"), bExists);

    if (bExists)
    {
        UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
        if (LoadedAsset)
        {
            Result->SetStringField(TEXT("asset_class"), LoadedAsset->GetClass()->GetName());
        }
    }

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetAssetInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
    if (!LoadedAsset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), LoadedAsset->GetName());
    Result->SetStringField(TEXT("path"), AssetPath);
    Result->SetStringField(TEXT("class"), LoadedAsset->GetClass()->GetName());

    // Static Mesh specific info
    UStaticMesh* StaticMesh = Cast<UStaticMesh>(LoadedAsset);
    if (StaticMesh)
    {
        if (StaticMesh->GetRenderData() && StaticMesh->GetRenderData()->LODResources.Num() > 0)
        {
            const FStaticMeshLODResources& LOD0 = StaticMesh->GetRenderData()->LODResources[0];
            Result->SetNumberField(TEXT("vertex_count"), LOD0.GetNumVertices());
            Result->SetNumberField(TEXT("triangle_count"), LOD0.GetNumTriangles());
        }

        Result->SetNumberField(TEXT("lod_count"), StaticMesh->GetRenderData() ? StaticMesh->GetRenderData()->LODResources.Num() : 0);
        Result->SetBoolField(TEXT("nanite_enabled"), StaticMesh->IsNaniteEnabled());

        // Material slots
        TArray<TSharedPtr<FJsonValue>> MaterialSlots;
        for (const FStaticMaterial& Mat : StaticMesh->GetStaticMaterials())
        {
            TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
            SlotObj->SetStringField(TEXT("name"), Mat.MaterialSlotName.ToString());
            SlotObj->SetStringField(TEXT("material"), Mat.MaterialInterface ? Mat.MaterialInterface->GetPathName() : TEXT("None"));
            MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotObj));
        }
        Result->SetArrayField(TEXT("material_slots"), MaterialSlots);

        // Bounding box
        FBox BoundingBox = StaticMesh->GetBoundingBox();
        TSharedPtr<FJsonObject> BoundsObj = MakeShared<FJsonObject>();
        BoundsObj->SetNumberField(TEXT("min_x"), BoundingBox.Min.X);
        BoundsObj->SetNumberField(TEXT("min_y"), BoundingBox.Min.Y);
        BoundsObj->SetNumberField(TEXT("min_z"), BoundingBox.Min.Z);
        BoundsObj->SetNumberField(TEXT("max_x"), BoundingBox.Max.X);
        BoundsObj->SetNumberField(TEXT("max_y"), BoundingBox.Max.Y);
        BoundsObj->SetNumberField(TEXT("max_z"), BoundingBox.Max.Z);
        Result->SetObjectField(TEXT("bounds"), BoundsObj);
    }

    // Texture specific info
    UTexture2D* Texture = Cast<UTexture2D>(LoadedAsset);
    if (Texture)
    {
        Result->SetNumberField(TEXT("width"), Texture->GetSizeX());
        Result->SetNumberField(TEXT("height"), Texture->GetSizeY());
        Result->SetStringField(TEXT("pixel_format"), GetPixelFormatString(Texture->GetPixelFormat()));
    }

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetHeightAtLocation(const TSharedPtr<FJsonObject>& Params)
{
    double X = 0.0, Y = 0.0;
    if (!Params->TryGetNumberField(TEXT("x"), X) || !Params->TryGetNumberField(TEXT("y"), Y))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'x' and/or 'y' parameters"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    FVector Start(X, Y, 100000.0);
    FVector End(X, Y, -100000.0);

    FCollisionQueryParams TraceParams(FName(TEXT("MCPHeightQuery")), true);
    TraceParams.bReturnPhysicalMaterial = false;

    FHitResult HitResult;
    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_WorldStatic,
        TraceParams
    );

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    if (bHit)
    {
        Result->SetBoolField(TEXT("success"), true);
        Result->SetNumberField(TEXT("x"), HitResult.Location.X);
        Result->SetNumberField(TEXT("y"), HitResult.Location.Y);
        Result->SetNumberField(TEXT("z"), HitResult.Location.Z);
        Result->SetStringField(TEXT("hit_actor"), HitResult.GetActor() ? HitResult.GetActor()->GetName() : TEXT("None"));
        Result->SetNumberField(TEXT("normal_x"), HitResult.ImpactNormal.X);
        Result->SetNumberField(TEXT("normal_y"), HitResult.ImpactNormal.Y);
        Result->SetNumberField(TEXT("normal_z"), HitResult.ImpactNormal.Z);
    }
    else
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("No surface found at location"));
    }

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSnapActorToGround(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    FVector ActorLocation = TargetActor->GetActorLocation();

    // Calculate VisualBottomOffset: distance from actor origin down to the lowest point.
    // For ACharacter: use CapsuleComponent directly (GetActorBounds includes SpringArm/Camera
    // which massively inflates bounds and causes characters to float).
    // For other actors: use GetActorBounds as before.
    float VisualBottomOffset;
    ACharacter* AsCharacter = Cast<ACharacter>(TargetActor);
    if (AsCharacter && AsCharacter->GetCapsuleComponent())
    {
        // ACharacter origin = capsule center. Capsule bottom = origin - ScaledHalfHeight.
        VisualBottomOffset = AsCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    }
    else
    {
        FVector Origin, BoxExtent;
        TargetActor->GetActorBounds(false, Origin, BoxExtent);
        VisualBottomOffset = ActorLocation.Z - (Origin.Z - BoxExtent.Z);
    }

    // Trace from high above the actor straight down
    FVector Start(ActorLocation.X, ActorLocation.Y, 100000.0);
    FVector End(ActorLocation.X, ActorLocation.Y, -100000.0);

    FCollisionQueryParams TraceParams(FName(TEXT("MCPSnapToGround")), true);
    TraceParams.AddIgnoredActor(TargetActor);

    FHitResult HitResult;
    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_WorldStatic,
        TraceParams
    );

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    if (bHit)
    {
        // Place actor so its bottom sits on the surface.
        FVector NewLocation = ActorLocation;
        NewLocation.Z = HitResult.Location.Z + VisualBottomOffset;

        TargetActor->SetActorLocation(NewLocation);

        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("actor"), ActorName);
        Result->SetNumberField(TEXT("old_z"), ActorLocation.Z);
        Result->SetNumberField(TEXT("new_z"), NewLocation.Z);
        Result->SetNumberField(TEXT("surface_z"), HitResult.Location.Z);
        Result->SetStringField(TEXT("hit_actor"), HitResult.GetActor() ? HitResult.GetActor()->GetName() : TEXT("None"));
    }
    else
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("No ground surface found below actor"));
    }

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleScatterMeshesOnLandscape(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    // Parse center point [X, Y]
    if (!Params->HasField(TEXT("center")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'center' parameter [x, y]"));
    }
    const TArray<TSharedPtr<FJsonValue>>& CenterArr = Params->GetArrayField(TEXT("center"));
    if (CenterArr.Num() < 2)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'center' must have at least 2 elements [x, y]"));
    }
    double CenterX = CenterArr[0]->AsNumber();
    double CenterY = CenterArr[1]->AsNumber();

    // Parse items array
    if (!Params->HasField(TEXT("items")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'items' array"));
    }
    const TArray<TSharedPtr<FJsonValue>>& Items = Params->GetArrayField(TEXT("items"));

    // Optional: delete existing actors with same names first
    bool bDeleteExisting = false;
    Params->TryGetBoolField(TEXT("delete_existing"), bDeleteExisting);

    // Randomization parameters for organic scatter
    double RandomOffset = 0.0;   // Random XY jitter range (± units)
    Params->TryGetNumberField(TEXT("random_offset"), RandomOffset);
    bool bRandomYaw = false;     // Full 360° random yaw per item
    Params->TryGetBoolField(TEXT("random_yaw"), bRandomYaw);
    double RandomScaleVariance = 0.0;  // ± fraction of scale (e.g. 0.2 = ±20%)
    Params->TryGetNumberField(TEXT("random_scale_variance"), RandomScaleVariance);

    TArray<TSharedPtr<FJsonValue>> ResultActors;
    TArray<FString> Errors;

    FCollisionQueryParams TraceParams(FName(TEXT("MCPScatterTrace")), true);
    TraceParams.bReturnPhysicalMaterial = false;

    for (const TSharedPtr<FJsonValue>& ItemVal : Items)
    {
        TSharedPtr<FJsonObject> Item = ItemVal->AsObject();
        if (!Item.IsValid()) continue;

        FString Name;
        if (!Item->TryGetStringField(TEXT("name"), Name))
        {
            Errors.Add(TEXT("Item missing 'name' field, skipped"));
            continue;
        }

        FString MeshPath;
        if (!Item->TryGetStringField(TEXT("static_mesh"), MeshPath))
        {
            Errors.Add(FString::Printf(TEXT("%s: missing 'static_mesh', skipped"), *Name));
            continue;
        }

        // Parse offset [dx, dy] in Unreal units
        double OffsetX = 0.0, OffsetY = 0.0;
        if (Item->HasField(TEXT("offset")))
        {
            const TArray<TSharedPtr<FJsonValue>>& OffArr = Item->GetArrayField(TEXT("offset"));
            if (OffArr.Num() >= 2)
            {
                OffsetX = OffArr[0]->AsNumber();
                OffsetY = OffArr[1]->AsNumber();
            }
        }

        // Apply random offset jitter
        if (RandomOffset > 0.0)
        {
            OffsetX += FMath::FRandRange(-RandomOffset, RandomOffset);
            OffsetY += FMath::FRandRange(-RandomOffset, RandomOffset);
        }

        // Parse rotation [Pitch, Yaw, Roll]
        FRotator Rotation(0, 0, 0);
        if (Item->HasField(TEXT("rotation")))
        {
            const TArray<TSharedPtr<FJsonValue>>& RotArr = Item->GetArrayField(TEXT("rotation"));
            if (RotArr.Num() >= 3)
            {
                Rotation.Pitch = RotArr[0]->AsNumber();
                Rotation.Yaw = RotArr[1]->AsNumber();
                Rotation.Roll = RotArr[2]->AsNumber();
            }
        }

        // Apply random yaw (full 360°) - also adds slight pitch/roll for natural tilt
        if (bRandomYaw)
        {
            Rotation.Yaw = FMath::FRandRange(0.0, 360.0);
            Rotation.Pitch += FMath::FRandRange(-3.0, 3.0);
            Rotation.Roll += FMath::FRandRange(-3.0, 3.0);
        }

        // Parse scale [X, Y, Z] or uniform
        FVector Scale(1, 1, 1);
        if (Item->HasField(TEXT("scale")))
        {
            const TArray<TSharedPtr<FJsonValue>>& ScaleArr = Item->GetArrayField(TEXT("scale"));
            if (ScaleArr.Num() >= 3)
            {
                Scale.X = ScaleArr[0]->AsNumber();
                Scale.Y = ScaleArr[1]->AsNumber();
                Scale.Z = ScaleArr[2]->AsNumber();
            }
            else if (ScaleArr.Num() == 1)
            {
                Scale = FVector(ScaleArr[0]->AsNumber());
            }
        }

        // Apply random scale variance (uniform, keeps proportions)
        if (RandomScaleVariance > 0.0)
        {
            double ScaleMult = 1.0 + FMath::FRandRange(-RandomScaleVariance, RandomScaleVariance);
            Scale *= ScaleMult;
        }

        // Calculate world position
        double WorldX = CenterX + OffsetX;
        double WorldY = CenterY + OffsetY;

        // Delete existing actor if requested
        if (bDeleteExisting)
        {
            TArray<AActor*> AllActors;
            UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
            for (AActor* Actor : AllActors)
            {
                if (IsValid(Actor) && Actor->GetName() == Name)
                {
                    // Use EditorActorSubsystem for safe editor deletion (handles OFPA packages)
                    UEditorActorSubsystem* EAS = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
                    if (EAS)
                    {
                        EAS->DestroyActor(Actor);
                    }
                    else
                    {
                        World->DestroyActor(Actor);
                    }
                    break;
                }
            }
        }

        // Line trace to find terrain height
        FVector TraceStart(WorldX, WorldY, 100000.0);
        FVector TraceEnd(WorldX, WorldY, -100000.0);
        FHitResult HitResult;

        bool bHit = World->LineTraceSingleByChannel(
            HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams
        );

        if (!bHit)
        {
            Errors.Add(FString::Printf(TEXT("%s: no surface at (%.1f, %.1f), skipped"), *Name, WorldX, WorldY));
            continue;
        }

        double SurfaceZ = HitResult.Location.Z;

        // Load mesh
        UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
        if (!Mesh)
        {
            Errors.Add(FString::Printf(TEXT("%s: mesh not found '%s', skipped"), *Name, *MeshPath));
            continue;
        }

        // Spawn actor with safe name mode (Requested = use name if free, auto-generate if taken)
        FVector Location(WorldX, WorldY, SurfaceZ);
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = *Name;
        SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

        AStaticMeshActor* NewActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams
        );

        if (!NewActor)
        {
            Errors.Add(FString::Printf(TEXT("%s: spawn failed"), *Name));
            continue;
        }

        NewActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        NewActor->SetActorScale3D(Scale);
        NewActor->SetFolderPath(TEXT("ScatteredMeshes"));

        // Build result entry
        TSharedPtr<FJsonObject> ActorResult = MakeShared<FJsonObject>();
        ActorResult->SetStringField(TEXT("name"), Name);
        ActorResult->SetStringField(TEXT("mesh"), MeshPath);
        ActorResult->SetNumberField(TEXT("x"), WorldX);
        ActorResult->SetNumberField(TEXT("y"), WorldY);
        ActorResult->SetNumberField(TEXT("z"), SurfaceZ);
        ActorResult->SetStringField(TEXT("surface_actor"), HitResult.GetActor() ? HitResult.GetActor()->GetName() : TEXT("None"));
        ResultActors.Add(MakeShared<FJsonValueObject>(ActorResult));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("placed_count"), ResultActors.Num());
    Result->SetArrayField(TEXT("actors"), ResultActors);

    if (Errors.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrArray;
        for (const FString& Err : Errors)
        {
            ErrArray.Add(MakeShared<FJsonValueString>(Err));
        }
        Result->SetArrayField(TEXT("errors"), ErrArray);
    }

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("file_path"), FilePath))
    {
        FilePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("MCP_Screenshot.png");
    }

    // Optional resolution (default 1920x1080)
    int32 Width = 1920;
    int32 Height = 1080;
    if (Params->HasField(TEXT("width")))
    {
        Width = FMath::Clamp(static_cast<int32>(Params->GetNumberField(TEXT("width"))), 320, 3840);
    }
    if (Params->HasField(TEXT("height")))
    {
        Height = FMath::Clamp(static_cast<int32>(Params->GetNumberField(TEXT("height"))), 240, 2160);
    }

    // Ensure directory exists
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FilePath));

    // === Step 1: Get viewport camera position, rotation, and FOV ===
    FVector CameraLocation = FVector::ZeroVector;
    FRotator CameraRotation = FRotator::ZeroRotator;
    float CameraFOV = 90.0f;
    bool bFoundCamera = false;
    FLevelEditorViewportClient* UsedClient = nullptr;

    if (GEditor)
    {
        // Prefer perspective level viewport
        const TArray<FLevelEditorViewportClient*>& LevelViewports = GEditor->GetLevelViewportClients();
        for (FLevelEditorViewportClient* ViewportClient : LevelViewports)
        {
            if (ViewportClient && ViewportClient->IsPerspective())
            {
                CameraLocation = ViewportClient->GetViewLocation();
                CameraRotation = ViewportClient->GetViewRotation();
                CameraFOV = ViewportClient->ViewFOV;
                UsedClient = ViewportClient;
                bFoundCamera = true;
                break;
            }
        }

        // Fallback: any level viewport (including ortho)
        if (!bFoundCamera)
        {
            for (FLevelEditorViewportClient* ViewportClient : LevelViewports)
            {
                if (ViewportClient)
                {
                    CameraLocation = ViewportClient->GetViewLocation();
                    CameraRotation = ViewportClient->GetViewRotation();
                    CameraFOV = ViewportClient->ViewFOV;
                    UsedClient = ViewportClient;
                    bFoundCamera = true;
                    break;
                }
            }
        }
    }

    if (!bFoundCamera)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("No editor viewport camera found. Is the level editor open?"));
    }

    // === Step 2: Get the editor world ===
    UWorld* World = nullptr;
    if (GEditor)
    {
        World = GEditor->GetEditorWorldContext().World();
    }
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    // === Step 3: Create off-screen render target ===
    // SceneCapture2D renders to its own texture — works even when the editor
    // viewport is minimized or has a zero-size backbuffer.
    UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
    RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, true);
    RenderTarget->UpdateResourceImmediate(false);

    // === Step 4: Spawn SceneCapture2D actor ===
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
    SpawnParams.ObjectFlags = RF_Transient; // Don't save this actor

    ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>(
        CameraLocation, CameraRotation, SpawnParams);

    if (!CaptureActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn SceneCapture2D actor"));
    }

    // === Step 5: Configure capture component ===
    USceneCaptureComponent2D* CaptureComp = CaptureActor->GetCaptureComponent2D();
    CaptureComp->TextureTarget = RenderTarget;
    CaptureComp->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    CaptureComp->bCaptureEveryFrame = false;
    CaptureComp->bCaptureOnMovement = false;
    CaptureComp->bAlwaysPersistRenderingState = true;
    CaptureComp->FOVAngle = CameraFOV;
    CaptureComp->HiddenActors.Add(CaptureActor);

    // Match the editor viewport's exposure so brightness matches what the user sees.
    // We do NOT copy the full EngineShowFlags because editor-specific flags
    // (grid, selection, widgets) break SceneCapture2D rendering.
    if (UsedClient)
    {
        FExposureSettings ExpSettings = UsedClient->ExposureSettings;
        if (ExpSettings.bFixed)
        {
            CaptureComp->PostProcessBlendWeight = 1.0f;
            CaptureComp->PostProcessSettings.bOverride_AutoExposureMethod = true;
            CaptureComp->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
            CaptureComp->PostProcessSettings.bOverride_AutoExposureBias = true;
            CaptureComp->PostProcessSettings.AutoExposureBias = ExpSettings.FixedEV100;
        }
    }

    // === Step 6: Capture the scene ===
    // CaptureScene() enqueues render commands. The subsequent ReadPixels()
    // internally calls FlushRenderingCommands() which blocks until the
    // capture render completes, giving us a synchronous result.
    CaptureComp->CaptureScene();

    // === Step 7: Read pixels from render target ===
    FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
    TArray<FColor> Bitmap;
    bool bReadOK = false;

    if (RTResource)
    {
        bReadOK = RTResource->ReadPixels(Bitmap);
    }

    // === Step 8: Cleanup capture actor ===
    UEditorActorSubsystem* ActorSub = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    if (ActorSub)
    {
        ActorSub->DestroyActor(CaptureActor);
    }

    if (!bReadOK)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Failed to read pixels from SceneCapture2D render target"));
    }

    // === Step 9: Encode to PNG via ImageWrapper ===
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create PNG image wrapper"));
    }

    if (!ImageWrapper->SetRaw(Bitmap.GetData(), Bitmap.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to set raw pixel data"));
    }

    TArray64<uint8> PNGData = ImageWrapper->GetCompressed();
    if (PNGData.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("PNG compression failed"));
    }

    if (!FFileHelper::SaveArrayToFile(PNGData, *FilePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to save screenshot to: %s"), *FilePath));
    }

    // Convert to absolute path for external tools
    FString AbsPath = FPaths::ConvertRelativePathToFull(FilePath);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("file_path"), AbsPath);
    Result->SetNumberField(TEXT("width"), Width);
    Result->SetNumberField(TEXT("height"), Height);
    Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Screenshot saved: %dx%d to %s"), Width, Height, *AbsPath));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    UMaterial* Material = Cast<UMaterial>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), Material->GetName());
    Result->SetStringField(TEXT("path"), MaterialPath);
    Result->SetBoolField(TEXT("two_sided"), Material->IsTwoSided());

    // Blend mode
    FString BlendMode;
    switch (Material->BlendMode)
    {
        case BLEND_Opaque: BlendMode = TEXT("Opaque"); break;
        case BLEND_Masked: BlendMode = TEXT("Masked"); break;
        case BLEND_Translucent: BlendMode = TEXT("Translucent"); break;
        case BLEND_Additive: BlendMode = TEXT("Additive"); break;
        case BLEND_Modulate: BlendMode = TEXT("Modulate"); break;
        default: BlendMode = TEXT("Unknown"); break;
    }
    Result->SetStringField(TEXT("blend_mode"), BlendMode);

    // Shading model
    FString ShadingModel;
    switch (Material->GetShadingModels().GetFirstShadingModel())
    {
        case MSM_DefaultLit: ShadingModel = TEXT("DefaultLit"); break;
        case MSM_Unlit: ShadingModel = TEXT("Unlit"); break;
        case MSM_Subsurface: ShadingModel = TEXT("Subsurface"); break;
        default: ShadingModel = TEXT("Other"); break;
    }
    Result->SetStringField(TEXT("shading_model"), ShadingModel);

    // Expressions
    TArray<TSharedPtr<FJsonValue>> ExprArray;
    for (UMaterialExpression* Expr : Material->GetExpressionCollection().Expressions)
    {
        if (!Expr) continue;
        TSharedPtr<FJsonObject> ExprInfo = MakeShared<FJsonObject>();
        ExprInfo->SetStringField(TEXT("class"), Expr->GetClass()->GetName());
        ExprInfo->SetStringField(TEXT("desc"), Expr->GetDescription());

        // TextureSample details
        UMaterialExpressionTextureSample* TexSample = Cast<UMaterialExpressionTextureSample>(Expr);
        if (TexSample)
        {
            ExprInfo->SetStringField(TEXT("texture"), TexSample->Texture ? TexSample->Texture->GetPathName() : TEXT("None"));
            FString SamplerStr;
            switch (TexSample->SamplerType)
            {
                case SAMPLERTYPE_Color: SamplerStr = TEXT("Color"); break;
                case SAMPLERTYPE_Normal: SamplerStr = TEXT("Normal"); break;
                case SAMPLERTYPE_Masks: SamplerStr = TEXT("Masks"); break;
                case SAMPLERTYPE_LinearColor: SamplerStr = TEXT("LinearColor"); break;
                case SAMPLERTYPE_Grayscale: SamplerStr = TEXT("Grayscale"); break;
                default: SamplerStr = TEXT("Unknown"); break;
            }
            ExprInfo->SetStringField(TEXT("sampler_type"), SamplerStr);
        }

        // ComponentMask details
        UMaterialExpressionComponentMask* Mask = Cast<UMaterialExpressionComponentMask>(Expr);
        if (Mask)
        {
            FString Channels;
            if (Mask->R) Channels += TEXT("R");
            if (Mask->G) Channels += TEXT("G");
            if (Mask->B) Channels += TEXT("B");
            if (Mask->A) Channels += TEXT("A");
            ExprInfo->SetStringField(TEXT("channels"), Channels);
        }

        ExprArray.Add(MakeShared<FJsonValueObject>(ExprInfo));
    }
    Result->SetArrayField(TEXT("expressions"), ExprArray);
    Result->SetNumberField(TEXT("expression_count"), ExprArray.Num());

    // Check which outputs are connected
    auto* EditorData = Material->GetEditorOnlyData();
    Result->SetBoolField(TEXT("base_color_connected"), EditorData->BaseColor.IsConnected());
    Result->SetBoolField(TEXT("normal_connected"), EditorData->Normal.IsConnected());
    Result->SetBoolField(TEXT("roughness_connected"), EditorData->Roughness.IsConnected());
    Result->SetBoolField(TEXT("metallic_connected"), EditorData->Metallic.IsConnected());
    Result->SetBoolField(TEXT("ao_connected"), EditorData->AmbientOcclusion.IsConnected());

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleFocusViewportOnActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Optional distance parameter
    double Distance = 500.0;
    Params->TryGetNumberField(TEXT("distance"), Distance);

    // Get actor bounds for framing
    FVector Origin, BoxExtent;
    TargetActor->GetActorBounds(false, Origin, BoxExtent);
    float MaxExtent = FMath::Max3(BoxExtent.X, BoxExtent.Y, BoxExtent.Z);
    if (MaxExtent < 50.0f) MaxExtent = 50.0f;

    // Calculate camera position: offset from actor center
    double ActualDistance = MaxExtent * 2.0 + Distance;
    FVector CamLocation = Origin + FVector(-ActualDistance * 0.7, -ActualDistance * 0.5, ActualDistance * 0.4);
    FRotator CamRotation = (Origin - CamLocation).Rotation();

    // Apply to the first level editor viewport (not asset editors, material previews, etc.)
    bool bApplied = false;
    const TArray<FLevelEditorViewportClient*>& LevelViewports = GEditor->GetLevelViewportClients();
    for (FLevelEditorViewportClient* ViewportClient : LevelViewports)
    {
        if (ViewportClient)
        {
            // Disable orbit camera and real-time override that may fight our position
            ViewportClient->SetViewLocation(CamLocation);
            ViewportClient->SetViewRotation(CamRotation);
            ViewportClient->Invalidate();
            // Force an immediate viewport redraw so subsequent screenshot captures the new view
            if (ViewportClient->Viewport)
            {
                ViewportClient->Viewport->Draw(false);
            }
            bApplied = true;
            break;
        }
    }

    // Fallback to any viewport client if no level viewport found
    if (!bApplied)
    {
        for (FEditorViewportClient* ViewportClient : GEditor->GetAllViewportClients())
        {
            if (ViewportClient)
            {
                ViewportClient->SetViewLocation(CamLocation);
                ViewportClient->SetViewRotation(CamRotation);
                ViewportClient->Invalidate();
                if (ViewportClient->Viewport)
                {
                    ViewportClient->Viewport->Draw(false);
                }
                bApplied = true;
                break;
            }
        }
    }

    if (!bApplied)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No viewport client found"));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("actor"), ActorName);
    Result->SetNumberField(TEXT("cam_x"), CamLocation.X);
    Result->SetNumberField(TEXT("cam_y"), CamLocation.Y);
    Result->SetNumberField(TEXT("cam_z"), CamLocation.Z);
    Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Viewport focused on %s"), *ActorName));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetTextureInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString TexturePath;
    if (!Params->TryGetStringField(TEXT("texture_path"), TexturePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'texture_path' parameter"));
    }

    UTexture2D* Texture = Cast<UTexture2D>(UEditorAssetLibrary::LoadAsset(TexturePath));
    if (!Texture)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Texture not found: %s"), *TexturePath));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), Texture->GetName());
    Result->SetStringField(TEXT("path"), TexturePath);
    Result->SetNumberField(TEXT("width"), Texture->GetSizeX());
    Result->SetNumberField(TEXT("height"), Texture->GetSizeY());
    Result->SetBoolField(TEXT("srgb"), Texture->SRGB);

    // Compression setting
    FString CompressionStr;
    switch (Texture->CompressionSettings)
    {
        case TC_Default: CompressionStr = TEXT("TC_Default"); break;
        case TC_Normalmap: CompressionStr = TEXT("TC_Normalmap"); break;
        case TC_Masks: CompressionStr = TEXT("TC_Masks"); break;
        case TC_Grayscale: CompressionStr = TEXT("TC_Grayscale"); break;
        case TC_HDR: CompressionStr = TEXT("TC_HDR"); break;
        default: CompressionStr = FString::Printf(TEXT("TC_%d"), (int32)Texture->CompressionSettings); break;
    }
    Result->SetStringField(TEXT("compression"), CompressionStr);
    Result->SetBoolField(TEXT("flip_green_channel"), Texture->bFlipGreenChannel);
    Result->SetNumberField(TEXT("num_mips"), Texture->GetNumMips());

    // LOD bias and group
    Result->SetNumberField(TEXT("lod_bias"), Texture->LODBias);

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDeleteActorsByPattern(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }

    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);

    // Phase 1: Collect matching actors (never modify during iteration)
    TArray<AActor*> ToDestroy;
    TArray<FString> DeletedNames;
    TArray<FString> FailedNames;

    for (AActor* Actor : AllActors)
    {
        if (!IsValid(Actor)) continue;
        FString ActorName = Actor->GetName();
        if (ActorName.Contains(Pattern))
        {
            ToDestroy.Add(Actor);
            DeletedNames.Add(ActorName);
        }
    }

    // Phase 2: Use EditorActorSubsystem for safe batch deletion
    // Handles OFPA packages, scene outliner, editor notifications
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    if (EditorActorSubsystem)
    {
        for (AActor* Actor : ToDestroy)
        {
            if (IsValid(Actor))
            {
                if (!EditorActorSubsystem->DestroyActor(Actor))
                {
                    FailedNames.Add(Actor->GetName());
                }
            }
        }
    }
    else
    {
        // Fallback: direct destroy (less safe in editor)
        for (AActor* Actor : ToDestroy)
        {
            if (IsValid(Actor))
            {
                if (!World->DestroyActor(Actor))
                {
                    FailedNames.Add(Actor->GetName());
                }
            }
        }
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("deleted_count"), DeletedNames.Num());
    Result->SetStringField(TEXT("pattern"), Pattern);

    TArray<TSharedPtr<FJsonValue>> DeletedArray;
    for (const FString& Name : DeletedNames)
    {
        DeletedArray.Add(MakeShared<FJsonValueString>(Name));
    }
    Result->SetArrayField(TEXT("deleted_actors"), DeletedArray);

    if (FailedNames.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> FailedArray;
        for (const FString& Name : FailedNames)
        {
            FailedArray.Add(MakeShared<FJsonValueString>(Name));
        }
        Result->SetArrayField(TEXT("failed_actors"), FailedArray);
    }

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleDeleteAsset(const TSharedPtr<FJsonObject>& Params)
{
    FString AssetPath;
    if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'asset_path' parameter"));
    }

    // Verify asset exists
    if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
    }

    bool bForceDelete = false;
    Params->TryGetBoolField(TEXT("force_delete"), bForceDelete);

    bool bCheckReferences = true;
    Params->TryGetBoolField(TEXT("check_references"), bCheckReferences);

    // Get asset info before deletion for the response
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
    if (!LoadedAsset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to load asset: %s"), *AssetPath));
    }

    FString AssetName = LoadedAsset->GetName();
    FString AssetClass = LoadedAsset->GetClass()->GetName();

    // Check for referencers if requested
    if (bCheckReferences)
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        // Get the package name for referencer lookup
        FName PackageName = FName(*LoadedAsset->GetOutermost()->GetName());
        TArray<FName> Referencers;
        AssetRegistry.GetReferencers(PackageName, Referencers);

        // Filter out self-references and script/engine references
        TArray<FString> RealReferencers;
        for (const FName& Ref : Referencers)
        {
            FString RefStr = Ref.ToString();
            // Skip self-reference, engine content, and script packages
            if (RefStr == PackageName.ToString()) continue;
            if (RefStr.StartsWith(TEXT("/Engine/"))) continue;
            if (RefStr.StartsWith(TEXT("/Script/"))) continue;
            RealReferencers.Add(RefStr);
        }

        if (RealReferencers.Num() > 0 && !bForceDelete)
        {
            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("success"), false);
            Result->SetStringField(TEXT("error"), TEXT("Asset has references. Use force_delete=true to delete anyway."));
            Result->SetStringField(TEXT("asset_path"), AssetPath);
            Result->SetStringField(TEXT("asset_class"), AssetClass);
            Result->SetNumberField(TEXT("referencer_count"), RealReferencers.Num());

            TArray<TSharedPtr<FJsonValue>> RefArray;
            for (const FString& Ref : RealReferencers)
            {
                RefArray.Add(MakeShared<FJsonValueString>(Ref));
            }
            Result->SetArrayField(TEXT("referencers"), RefArray);
            return Result;
        }
    }

    // Perform the deletion
    bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);

    if (!bDeleted)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to delete asset: %s"), *AssetPath));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("deleted_asset"), AssetPath);
    Result->SetStringField(TEXT("asset_name"), AssetName);
    Result->SetStringField(TEXT("asset_class"), AssetClass);
    Result->SetStringField(TEXT("message"), FString::Printf(TEXT("Asset '%s' deleted successfully"), *AssetName));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleSetNaniteEnabled(const TSharedPtr<FJsonObject>& Params)
{
    FString MeshPath = Params->GetStringField(TEXT("mesh_path"));
    bool bEnabled = Params->HasField(TEXT("enabled")) ? Params->GetBoolField(TEXT("enabled")) : false;

    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
    if (!Mesh)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Static mesh not found: %s"), *MeshPath));
    }

    FMeshNaniteSettings NaniteSettings = Mesh->GetNaniteSettings();
    bool bWasEnabled = NaniteSettings.bEnabled;
    NaniteSettings.bEnabled = bEnabled;
    Mesh->SetNaniteSettings(NaniteSettings);
    Mesh->PostEditChange();
    Mesh->MarkPackageDirty();

    // Save to disk
    UEditorAssetLibrary::SaveLoadedAsset(Mesh);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("mesh_path"), MeshPath);
    Result->SetBoolField(TEXT("nanite_enabled"), bEnabled);
    Result->SetBoolField(TEXT("was_enabled"), bWasEnabled);
    Result->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Nanite %s on %s (was %s)"),
            bEnabled ? TEXT("enabled") : TEXT("disabled"),
            *Mesh->GetName(),
            bWasEnabled ? TEXT("enabled") : TEXT("disabled")));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleScatterFoliage(const TSharedPtr<FJsonObject>& Params)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world available"));
    }

    // --- Parse required parameters ---
    FString MeshPath;
    if (!Params->TryGetStringField(TEXT("mesh_path"), MeshPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'mesh_path' parameter"));
    }

    if (!Params->HasField(TEXT("center")))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'center' parameter [x, y]"));
    }
    const TArray<TSharedPtr<FJsonValue>>& CenterArr = Params->GetArrayField(TEXT("center"));
    if (CenterArr.Num() < 2)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'center' must have at least 2 elements [x, y]"));
    }
    double CenterX = CenterArr[0]->AsNumber();
    double CenterY = CenterArr[1]->AsNumber();

    double Radius = 5000.0;
    Params->TryGetNumberField(TEXT("radius"), Radius);
    if (Radius <= 0.0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'radius' must be positive"));
    }

    // --- Parse optional rectangular bounds (overrides center+radius) ---
    bool bUseRectBounds = false;
    double BoundsMinX = 0.0, BoundsMaxX = 0.0, BoundsMinY = 0.0, BoundsMaxY = 0.0;

    if (Params->HasField(TEXT("bounds")))
    {
        const TArray<TSharedPtr<FJsonValue>>& BoundsArr = Params->GetArrayField(TEXT("bounds"));
        if (BoundsArr.Num() >= 4)
        {
            BoundsMinX = BoundsArr[0]->AsNumber();
            BoundsMaxX = BoundsArr[1]->AsNumber();
            BoundsMinY = BoundsArr[2]->AsNumber();
            BoundsMaxY = BoundsArr[3]->AsNumber();
            bUseRectBounds = true;

            // Override center and radius for grid calculations
            CenterX = (BoundsMinX + BoundsMaxX) * 0.5;
            CenterY = (BoundsMinY + BoundsMaxY) * 0.5;
            // Radius still needed for grid sizing - use half-diagonal
            double HalfW = (BoundsMaxX - BoundsMinX) * 0.5;
            double HalfH = (BoundsMaxY - BoundsMinY) * 0.5;
            Radius = FMath::Sqrt(HalfW * HalfW + HalfH * HalfH);
        }
    }

    // --- Parse optional parameters ---
    int32 Count = 100;
    double CountD = 100.0;
    if (Params->TryGetNumberField(TEXT("count"), CountD))
    {
        Count = FMath::Clamp((int32)CountD, 1, 50000);
    }

    double MinDistance = 50.0;
    Params->TryGetNumberField(TEXT("min_distance"), MinDistance);
    MinDistance = FMath::Max(MinDistance, 1.0);

    double MaxSlope = 30.0;
    Params->TryGetNumberField(TEXT("max_slope"), MaxSlope);

    bool bAlignToSurface = false;
    Params->TryGetBoolField(TEXT("align_to_surface"), bAlignToSurface);

    bool bRandomYaw = true;
    Params->TryGetBoolField(TEXT("random_yaw"), bRandomYaw);

    double ScaleMin = 1.0, ScaleMax = 1.0;
    if (Params->HasField(TEXT("scale_range")))
    {
        const TArray<TSharedPtr<FJsonValue>>& ScaleArr = Params->GetArrayField(TEXT("scale_range"));
        if (ScaleArr.Num() >= 2)
        {
            ScaleMin = ScaleArr[0]->AsNumber();
            ScaleMax = ScaleArr[1]->AsNumber();
        }
    }

    double ZOffset = 0.0;
    Params->TryGetNumberField(TEXT("z_offset"), ZOffset);

    FString ActorName = TEXT("HISM_Foliage");
    Params->TryGetStringField(TEXT("actor_name"), ActorName);

    double CullDistance = 0.0;
    Params->TryGetNumberField(TEXT("cull_distance"), CullDistance);

    FString MaterialPath;
    Params->TryGetStringField(TEXT("material_path"), MaterialPath);

    TArray<FString> MaterialPaths;
    const TArray<TSharedPtr<FJsonValue>>* MaterialsArray = nullptr;
    if (Params->TryGetArrayField(TEXT("materials"), MaterialsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *MaterialsArray)
        {
            MaterialPaths.Add(Val->AsString());
        }
    }

    // --- Load mesh ---
    UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
    if (!Mesh)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Static mesh not found: %s"), *MeshPath));
    }

    // --- Phase A: Poisson Disk Sampling (grid-accelerated) ---
    // Grid cell size = MinDistance / sqrt(2) for optimal Poisson disk coverage
    const double CellSize = MinDistance / FMath::Sqrt(2.0);
    double AreaSizeX, AreaSizeY;
    int32 GridDimX, GridDimY;

    if (bUseRectBounds)
    {
        AreaSizeX = BoundsMaxX - BoundsMinX;
        AreaSizeY = BoundsMaxY - BoundsMinY;
    }
    else
    {
        AreaSizeX = Radius * 2.0;
        AreaSizeY = Radius * 2.0;
    }
    GridDimX = FMath::CeilToInt(AreaSizeX / CellSize);
    GridDimY = FMath::CeilToInt(AreaSizeY / CellSize);

    // Safety cap: 4M cells max (~16MB memory)
    if ((int64)GridDimX * GridDimY > 4000000)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Grid too large: %dx%d cells. Increase min_distance or decrease area."), GridDimX, GridDimY));
    }

    // Grid stores index into Points array (-1 = empty)
    TArray<int32> Grid;
    Grid.Init(-1, GridDimX * GridDimY);

    struct FPoint2D { double X; double Y; };
    TArray<FPoint2D> Points;
    Points.Reserve(Count);

    // Active list for dart-throwing
    TArray<int32> ActiveList;
    ActiveList.Reserve(Count);

    // Grid origin: bottom-left corner of the scatter area
    double GridOriginX = bUseRectBounds ? BoundsMinX : (CenterX - Radius);
    double GridOriginY = bUseRectBounds ? BoundsMinY : (CenterY - Radius);

    // Lambda to get grid index from world-relative position
    auto GridIndex = [&](double PX, double PY) -> int32
    {
        int32 GX = FMath::Clamp((int32)((PX - GridOriginX) / CellSize), 0, GridDimX - 1);
        int32 GY = FMath::Clamp((int32)((PY - GridOriginY) / CellSize), 0, GridDimY - 1);
        return GY * GridDimX + GX;
    };

    // Seed initial points
    if (bUseRectBounds)
    {
        // Multi-seed: place seed points RANDOMLY across the rectangle.
        // Random placement eliminates grid artifacts that appear when seeds
        // are on a regular grid (growth fronts create Voronoi-like cells).
        double SeedSpacing = MinDistance * 5.0;
        int32 TargetSeeds = FMath::Max(1, FMath::CeilToInt((AreaSizeX * AreaSizeY) / (SeedSpacing * SeedSpacing)));
        int32 MaxSeedAttempts = TargetSeeds * 10; // Extra attempts for rejections

        for (int32 Attempt = 0; Attempt < MaxSeedAttempts && ActiveList.Num() < TargetSeeds; ++Attempt)
        {
            double PX = FMath::FRandRange(BoundsMinX, BoundsMaxX);
            double PY = FMath::FRandRange(BoundsMinY, BoundsMaxY);

            // Check this cell and neighbors for min_distance from existing seeds
            int32 CandGX = FMath::Clamp((int32)((PX - GridOriginX) / CellSize), 0, GridDimX - 1);
            int32 CandGY = FMath::Clamp((int32)((PY - GridOriginY) / CellSize), 0, GridDimY - 1);
            int32 GIdx = CandGY * GridDimX + CandGX;

            if (Grid[GIdx] >= 0) continue; // Cell already occupied

            bool bTooClose = false;
            for (int32 NY = FMath::Max(0, CandGY - 2); NY <= FMath::Min(GridDimY - 1, CandGY + 2) && !bTooClose; ++NY)
            {
                for (int32 NX = FMath::Max(0, CandGX - 2); NX <= FMath::Min(GridDimX - 1, CandGX + 2) && !bTooClose; ++NX)
                {
                    int32 NIdx = NY * GridDimX + NX;
                    if (Grid[NIdx] >= 0)
                    {
                        const FPoint2D& Neighbor = Points[Grid[NIdx]];
                        double NDX = PX - Neighbor.X;
                        double NDY = PY - Neighbor.Y;
                        if (NDX * NDX + NDY * NDY < MinDistance * MinDistance)
                        {
                            bTooClose = true;
                        }
                    }
                }
            }

            if (!bTooClose)
            {
                FPoint2D P = { PX, PY };
                int32 Idx = Points.Add(P);
                ActiveList.Add(Idx);
                Grid[GIdx] = Idx;
            }
        }
    }
    else
    {
        // Circular mode: single seed at center
        FPoint2D P = { CenterX, CenterY };
        int32 Idx = Points.Add(P);
        ActiveList.Add(Idx);
        Grid[GridIndex(P.X, P.Y)] = Idx;
    }

    const int32 MaxAttempts = 30; // Standard Poisson disk attempts per point

    while (ActiveList.Num() > 0 && Points.Num() < Count)
    {
        // Pick random active point
        int32 ActiveIdx = FMath::RandRange(0, ActiveList.Num() - 1);
        int32 PointIdx = ActiveList[ActiveIdx];
        const FPoint2D& BasePoint = Points[PointIdx];

        bool bFoundCandidate = false;
        for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
        {
            // Generate candidate in annulus [MinDistance, 2*MinDistance]
            double Angle = FMath::FRandRange(0.0, 2.0 * PI);
            double Dist = FMath::FRandRange(MinDistance, 2.0 * MinDistance);
            double CandX = BasePoint.X + Dist * FMath::Cos(Angle);
            double CandY = BasePoint.Y + Dist * FMath::Sin(Angle);

            // Check bounds
            if (bUseRectBounds)
            {
                if (CandX < BoundsMinX || CandX > BoundsMaxX || CandY < BoundsMinY || CandY > BoundsMaxY)
                {
                    continue;
                }
            }
            else
            {
                double DX = CandX - CenterX;
                double DY = CandY - CenterY;
                if (DX * DX + DY * DY > Radius * Radius)
                {
                    continue;
                }
            }

            // Check grid neighbors (5x5 around candidate cell)
            int32 CandGX = FMath::Clamp((int32)((CandX - GridOriginX) / CellSize), 0, GridDimX - 1);
            int32 CandGY = FMath::Clamp((int32)((CandY - GridOriginY) / CellSize), 0, GridDimY - 1);

            bool bTooClose = false;
            for (int32 NY = FMath::Max(0, CandGY - 2); NY <= FMath::Min(GridDimY - 1, CandGY + 2) && !bTooClose; ++NY)
            {
                for (int32 NX = FMath::Max(0, CandGX - 2); NX <= FMath::Min(GridDimX - 1, CandGX + 2) && !bTooClose; ++NX)
                {
                    int32 NIdx = NY * GridDimX + NX;
                    if (Grid[NIdx] >= 0)
                    {
                        const FPoint2D& Neighbor = Points[Grid[NIdx]];
                        double NDX = CandX - Neighbor.X;
                        double NDY = CandY - Neighbor.Y;
                        if (NDX * NDX + NDY * NDY < MinDistance * MinDistance)
                        {
                            bTooClose = true;
                        }
                    }
                }
            }

            if (!bTooClose)
            {
                FPoint2D NewPoint = { CandX, CandY };
                int32 NewIdx = Points.Add(NewPoint);
                ActiveList.Add(NewIdx);
                Grid[GridIndex(CandX, CandY)] = NewIdx;
                bFoundCandidate = true;
                break;
            }
        }

        if (!bFoundCandidate)
        {
            // Remove exhausted point from active list
            ActiveList.RemoveAtSwap(ActiveIdx);
        }
    }

    // --- Phase B: Line trace each point, filter by slope ---
    struct FInstanceData
    {
        FVector Location;
        FRotator Rotation;
        FVector Scale;
    };
    TArray<FInstanceData> ValidInstances;
    ValidInstances.Reserve(Points.Num());

    FCollisionQueryParams TraceParams(FName(TEXT("MCPFoliageScatterTrace")), true);
    TraceParams.bReturnPhysicalMaterial = false;

    int32 RejectedSlope = 0;
    int32 RejectedNoHit = 0;

    double MaxSlopeRad = FMath::DegreesToRadians(MaxSlope);
    double MaxSlopeCosine = FMath::Cos(MaxSlopeRad);

    for (const FPoint2D& Pt : Points)
    {
        FVector TraceStart(Pt.X, Pt.Y, 100000.0);
        FVector TraceEnd(Pt.X, Pt.Y, -100000.0);
        FHitResult HitResult;

        bool bHit = World->LineTraceSingleByChannel(
            HitResult, TraceStart, TraceEnd, ECC_WorldStatic, TraceParams
        );

        if (!bHit)
        {
            ++RejectedNoHit;
            continue;
        }

        // Only place on landscape surfaces - reject hits on rocks/meshes
        AActor* HitActor = HitResult.GetActor();
        if (HitActor && !HitActor->IsA(ALandscapeProxy::StaticClass()))
        {
            ++RejectedNoHit;
            continue;
        }

        // Check slope: dot(Normal, Up) gives cosine of slope angle
        // Normal.Z == cos(slope_angle), reject if angle > MaxSlope
        double SlopeCosine = HitResult.ImpactNormal.Z;
        if (SlopeCosine < MaxSlopeCosine)
        {
            ++RejectedSlope;
            continue;
        }

        // Build instance transform
        FInstanceData Inst;
        Inst.Location = HitResult.Location + FVector(0, 0, ZOffset);

        // Rotation
        if (bAlignToSurface)
        {
            // Align Z-axis to surface normal
            FVector Up = HitResult.ImpactNormal;
            FVector Forward = FVector::CrossProduct(FVector::RightVector, Up);
            if (Forward.IsNearlyZero())
            {
                Forward = FVector::CrossProduct(FVector::ForwardVector, Up);
            }
            Forward.Normalize();
            FVector Right = FVector::CrossProduct(Up, Forward);
            Inst.Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
        }
        else
        {
            Inst.Rotation = FRotator::ZeroRotator;
        }

        if (bRandomYaw)
        {
            Inst.Rotation.Yaw = FMath::FRandRange(0.0, 360.0);
        }

        // Scale
        double UniformScale = FMath::FRandRange(ScaleMin, ScaleMax);
        Inst.Scale = FVector(UniformScale);

        ValidInstances.Add(Inst);
    }

    if (ValidInstances.Num() == 0)
    {
        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"), true);
        Result->SetNumberField(TEXT("instance_count"), 0);
        Result->SetNumberField(TEXT("candidates_generated"), Points.Num());
        Result->SetNumberField(TEXT("rejected_slope"), RejectedSlope);
        Result->SetNumberField(TEXT("rejected_no_hit"), RejectedNoHit);
        Result->SetStringField(TEXT("message"), TEXT("No valid placement positions found after filtering"));
        return Result;
    }

    // --- Phase C: Create AActor + HISM component, batch AddInstances ---
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

    AActor* ContainerActor = World->SpawnActor<AActor>(
        AActor::StaticClass(), FVector(CenterX, CenterY, 0), FRotator::ZeroRotator, SpawnParams
    );

    if (!ContainerActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn container actor"));
    }

    // Mark actor for editor serialization (undo/redo + save)
    ContainerActor->SetFlags(RF_Transactional);

    // Set root component
    USceneComponent* RootComp = NewObject<USceneComponent>(ContainerActor, TEXT("Root"));
    RootComp->SetFlags(RF_Transactional);
    ContainerActor->SetRootComponent(RootComp);
    RootComp->RegisterComponent();

    // Create HISM component with persistence flags
    UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
        ContainerActor, *FString::Printf(TEXT("HISM_%s"), *Mesh->GetName())
    );
    HISM->SetFlags(RF_Transactional);
    HISM->CreationMethod = EComponentCreationMethod::Instance;
    HISM->SetStaticMesh(Mesh);
    HISM->SetMobility(EComponentMobility::Static);
    HISM->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);

    // Apply per-slot materials if provided, otherwise fall back to single material_path
    if (MaterialPaths.Num() > 0)
    {
        int32 NumSlots = Mesh->GetStaticMaterials().Num();
        for (int32 MatIdx = 0; MatIdx < FMath::Min(MaterialPaths.Num(), NumSlots); ++MatIdx)
        {
            if (!MaterialPaths[MatIdx].IsEmpty())
            {
                UMaterialInterface* Mat = Cast<UMaterialInterface>(
                    UEditorAssetLibrary::LoadAsset(MaterialPaths[MatIdx]));
                if (Mat)
                {
                    HISM->SetMaterial(MatIdx, Mat);
                }
            }
        }
    }
    else if (!MaterialPath.IsEmpty())
    {
        UMaterialInterface* MatOverride = Cast<UMaterialInterface>(
            UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (MatOverride)
        {
            for (int32 MatIdx = 0; MatIdx < Mesh->GetStaticMaterials().Num(); ++MatIdx)
            {
                HISM->SetMaterial(MatIdx, MatOverride);
            }
        }
    }

    // Set cull distance
    if (CullDistance > 0.0)
    {
        HISM->SetCullDistances((int32)0, (int32)CullDistance);
    }

    HISM->RegisterComponent();

    // Register as instance component so it serializes with the actor
    ContainerActor->AddInstanceComponent(HISM);

    // Disable auto-rebuild during batch add (rebuild manually after)
    HISM->bAutoRebuildTreeOnInstanceChanges = false;

    // Build transform array and batch-add instances
    TArray<FTransform> Transforms;
    Transforms.Reserve(ValidInstances.Num());

    for (const FInstanceData& Inst : ValidInstances)
    {
        FTransform T(Inst.Rotation.Quaternion(), Inst.Location, Inst.Scale);
        Transforms.Add(T);
    }

    // Mark for modification BEFORE changing data (enables undo/redo tracking)
    HISM->Modify();

    // AddInstances with world-space transforms
    HISM->AddInstances(Transforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/true);

    // Rebuild HISM cluster tree (must happen before save or reload will crash)
    HISM->BuildTreeIfOutdated(true, true);
    HISM->bAutoRebuildTreeOnInstanceChanges = true;

    // Notify editor that PerInstanceSMData changed (triggers serialization)
    FProperty* PerInstanceProp = FindFieldChecked<FProperty>(
        UInstancedStaticMeshComponent::StaticClass(),
        GET_MEMBER_NAME_CHECKED(UInstancedStaticMeshComponent, PerInstanceSMData)
    );
    FPropertyChangedEvent PropertyEvent(PerInstanceProp);
    HISM->PostEditChangeProperty(PropertyEvent);

    // Mark HISM package dirty
    HISM->MarkPackageDirty();

    // Organize in editor
    ContainerActor->SetFolderPath(TEXT("Foliage"));
    ContainerActor->Modify();
    ContainerActor->MarkPackageDirty();

    // Handle OFPA (One File Per Actor) external package
    if (UPackage* ExtPackage = ContainerActor->GetExternalPackage())
    {
        ExtPackage->SetDirtyFlag(true);
    }

    // --- Build response ---
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("actor_name"), ContainerActor->GetName());
    Result->SetStringField(TEXT("mesh"), MeshPath);
    Result->SetNumberField(TEXT("instance_count"), ValidInstances.Num());
    Result->SetNumberField(TEXT("candidates_generated"), Points.Num());
    Result->SetNumberField(TEXT("rejected_slope"), RejectedSlope);
    Result->SetNumberField(TEXT("rejected_no_hit"), RejectedNoHit);
    Result->SetNumberField(TEXT("center_x"), CenterX);
    Result->SetNumberField(TEXT("center_y"), CenterY);
    Result->SetNumberField(TEXT("radius"), Radius);
    if (bUseRectBounds)
    {
        TArray<TSharedPtr<FJsonValue>> BoundsJsonArr;
        BoundsJsonArr.Add(MakeShared<FJsonValueNumber>(BoundsMinX));
        BoundsJsonArr.Add(MakeShared<FJsonValueNumber>(BoundsMaxX));
        BoundsJsonArr.Add(MakeShared<FJsonValueNumber>(BoundsMinY));
        BoundsJsonArr.Add(MakeShared<FJsonValueNumber>(BoundsMaxY));
        Result->SetArrayField(TEXT("bounds"), BoundsJsonArr);
    }
    Result->SetStringField(TEXT("message"),
        FString::Printf(TEXT("Scattered %d instances of %s via HISM (Poisson disk, %d candidates, %d slope-rejected, %d no-hit)"),
            ValidInstances.Num(), *Mesh->GetName(), Points.Num(), RejectedSlope, RejectedNoHit));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleImportSound(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString SourcePath;
    if (!Params->TryGetStringField(TEXT("source_path"), SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_path' parameter"));
    }

    FString SoundName;
    if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
    {
        // Use source filename if not provided
        SoundName = FPaths::GetBaseFilename(SourcePath);
    }

    // Get optional destination path
    FString DestinationPath = TEXT("/Game/Audio/");
    Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

    if (!DestinationPath.EndsWith(TEXT("/")))
    {
        DestinationPath += TEXT("/");
    }

    // Check if source file exists
    if (!FPaths::FileExists(SourcePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Source file not found: %s"), *SourcePath));
    }

    // Create package for the sound
    FString PackagePath = DestinationPath + SoundName;

    // If asset already exists, return success — NEVER delete+reimport over loaded assets
    // (editor subsystems hold RefCount > 0, causing "partially loaded" crash on SavePackage)
    if (UEditorAssetLibrary::DoesAssetExist(PackagePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("import_sound: Asset '%s' already exists, skipping import"), *PackagePath);
        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("success"), true);
        Result->SetStringField(TEXT("name"), SoundName);
        Result->SetStringField(TEXT("path"), PackagePath);
        Result->SetStringField(TEXT("message"), TEXT("Asset already exists, skipped import"));
        return Result;
    }

    UPackage* Package = CreatePackage(*PackagePath);

    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package for sound"));
    }

    // Create sound factory
    USoundFactory* SoundFactory = NewObject<USoundFactory>();
    SoundFactory->AddToRoot();

    // Import the sound
    bool bCancelled = false;
    USoundWave* ImportedSound = Cast<USoundWave>(SoundFactory->ImportObject(
        USoundWave::StaticClass(),
        Package,
        FName(*SoundName),
        RF_Public | RF_Standalone,
        SourcePath,
        nullptr,
        bCancelled
    ));

    SoundFactory->RemoveFromRoot();

    if (!ImportedSound)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to import sound from: %s"), *SourcePath));
    }

    // Apply optional properties
    bool bLooping = false;
    if (Params->TryGetBoolField(TEXT("looping"), bLooping))
    {
        ImportedSound->bLooping = bLooping;
    }

    double Volume = 1.0;
    if (Params->TryGetNumberField(TEXT("volume"), Volume))
    {
        ImportedSound->Volume = static_cast<float>(Volume);
    }

    ImportedSound->PostEditChange();

    // Notify asset registry
    IAssetRegistry::Get()->AssetCreated(ImportedSound);

    // Save package to disk immediately (same pattern as texture import)
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, ImportedSound, *PackageFilename, SaveArgs);

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), SoundName);
    Result->SetStringField(TEXT("path"), PackagePath);
    Result->SetStringField(TEXT("source"), SourcePath);
    Result->SetNumberField(TEXT("duration_seconds"), ImportedSound->Duration);
    Result->SetNumberField(TEXT("sample_rate"), static_cast<double>(ImportedSound->GetSampleRateForCurrentPlatform()));
    Result->SetNumberField(TEXT("num_channels"), static_cast<double>(ImportedSound->NumChannels));
    Result->SetBoolField(TEXT("looping"), ImportedSound->bLooping);
    Result->SetStringField(TEXT("message"), TEXT("Sound imported successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleAddAnimNotify(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimationPath;
    if (!Params->TryGetStringField(TEXT("animation_path"), AnimationPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'animation_path' parameter"));
    }

    double TimeSeconds = 0.0;
    if (!Params->TryGetNumberField(TEXT("time_seconds"), TimeSeconds))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'time_seconds' parameter"));
    }

    FString SoundPath;
    if (!Params->TryGetStringField(TEXT("sound_path"), SoundPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'sound_path' parameter"));
    }

    double Volume = 1.0;
    Params->TryGetNumberField(TEXT("volume"), Volume);

    bool bClearExisting = false;
    Params->TryGetBoolField(TEXT("clear_existing"), bClearExisting);

    // Load the animation sequence
    FString FullAnimPath = AnimationPath;
    if (!FullAnimPath.Contains(TEXT(".")))
    {
        FString AssetName = FPackageName::GetShortName(FullAnimPath);
        FullAnimPath = FullAnimPath + TEXT(".") + AssetName;
    }

    UAnimSequence* AnimSeq = LoadObject<UAnimSequence>(nullptr, *FullAnimPath);
    if (!AnimSeq)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Animation not found: %s"), *AnimationPath));
    }

    // Load the sound asset
    FString FullSoundPath = SoundPath;
    if (!FullSoundPath.Contains(TEXT(".")))
    {
        FString SoundName = FPackageName::GetShortName(FullSoundPath);
        FullSoundPath = FullSoundPath + TEXT(".") + SoundName;
    }

    USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FullSoundPath);
    if (!Sound)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Sound not found: %s"), *SoundPath));
    }

    // Clear existing notifies if requested
    if (bClearExisting)
    {
        AnimSeq->Notifies.Empty();
    }

    // Create the PlaySound notify instance
    UAnimNotify_PlaySound* SoundNotify = NewObject<UAnimNotify_PlaySound>(AnimSeq);
    SoundNotify->Sound = Sound;
    SoundNotify->VolumeMultiplier = static_cast<float>(Volume);

    // Add notify event to the animation
    float ClampedTime = FMath::Clamp(static_cast<float>(TimeSeconds), 0.02f, AnimSeq->GetPlayLength() - 0.01f);

    FAnimNotifyEvent& NewEvent = AnimSeq->Notifies.AddDefaulted_GetRef();
    NewEvent.Notify = SoundNotify;
    NewEvent.NotifyName = FName(TEXT("PlaySound"));
    NewEvent.Link(AnimSeq, ClampedTime);
    NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);

    // Sort notifies by time and refresh cache
    AnimSeq->SortNotifies();
    AnimSeq->PostEditChange();
    AnimSeq->MarkPackageDirty();

    // Save to disk
    UPackage* Package = AnimSeq->GetOutermost();
    FString PackageName = Package->GetName();
    FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, AnimSeq, *PackageFilename, SaveArgs);

    // Result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("animation_path"), AnimationPath);
    Result->SetStringField(TEXT("sound_path"), SoundPath);
    Result->SetNumberField(TEXT("time_seconds"), ClampedTime);
    Result->SetNumberField(TEXT("total_notifies"), AnimSeq->Notifies.Num());
    Result->SetNumberField(TEXT("animation_length"), AnimSeq->GetPlayLength());
    Result->SetStringField(TEXT("message"), TEXT("AnimNotify_PlaySound added successfully"));

    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPEditorCommands::HandleGetEditorLog(const TSharedPtr<FJsonObject>& Params)
{
    // Parameters: num_lines (default 200), filter (optional substring match)
    int32 NumLines = 200;
    double NumLinesDouble = 0;
    if (Params->TryGetNumberField(TEXT("num_lines"), NumLinesDouble))
    {
        NumLines = FMath::Clamp(static_cast<int32>(NumLinesDouble), 1, 5000);
    }

    FString Filter;
    Params->TryGetStringField(TEXT("filter"), Filter);

    // Find the current log file
    FString LogDir = FPaths::ProjectLogDir();
    FString LogFilePath = LogDir / FApp::GetProjectName() + TEXT(".log");

    if (!FPaths::FileExists(LogFilePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Log file not found at: %s"), *LogFilePath));
    }

    // Read the entire log file
    FString LogContent;
    if (!FFileHelper::LoadFileToString(LogContent, *LogFilePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Failed to read log file: %s"), *LogFilePath));
    }

    // Split into lines
    TArray<FString> AllLines;
    LogContent.ParseIntoArrayLines(AllLines);

    // Apply filter if provided
    TArray<FString> FilteredLines;
    if (Filter.IsEmpty())
    {
        // No filter — take last N lines
        int32 StartIdx = FMath::Max(0, AllLines.Num() - NumLines);
        for (int32 i = StartIdx; i < AllLines.Num(); i++)
        {
            FilteredLines.Add(AllLines[i]);
        }
    }
    else
    {
        // Filter matching lines, then take last N
        TArray<FString> MatchingLines;
        for (const FString& Line : AllLines)
        {
            if (Line.Contains(Filter))
            {
                MatchingLines.Add(Line);
            }
        }
        int32 StartIdx = FMath::Max(0, MatchingLines.Num() - NumLines);
        for (int32 i = StartIdx; i < MatchingLines.Num(); i++)
        {
            FilteredLines.Add(MatchingLines[i]);
        }
    }

    // Build result
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("log_file"), LogFilePath);
    Result->SetNumberField(TEXT("total_lines"), AllLines.Num());
    Result->SetNumberField(TEXT("returned_lines"), FilteredLines.Num());

    if (!Filter.IsEmpty())
    {
        Result->SetStringField(TEXT("filter"), Filter);
    }

    // Join filtered lines into a single string (more compact than JSON array)
    FString JoinedLines = FString::Join(FilteredLines, TEXT("\n"));
    Result->SetStringField(TEXT("lines"), JoinedLines);

    return Result;
}
