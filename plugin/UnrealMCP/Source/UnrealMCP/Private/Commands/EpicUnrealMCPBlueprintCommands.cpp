#include "Commands/EpicUnrealMCPBlueprintCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/Engine.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
// Character Blueprint includes
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "UObject/SavePackage.h"
// Animation Blueprint includes
#include "Factories/AnimBlueprintFactory.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstance.h"
// AnimGraph State Machine includes
#include "AnimGraphNode_StateMachine.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_SequencePlayer.h"
#include "AnimGraphNode_StateResult.h"
#include "AnimGraphNode_TransitionResult.h"
#include "AnimationStateMachineGraph.h"
#include "AnimationStateMachineSchema.h"
#include "AnimStateNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimStateEntryNode.h"
#include "AnimationStateGraph.h"
#include "AnimationTransitionGraph.h"
#include "Animation/AnimSequence.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "Kismet/KismetMathLibrary.h"
// BlendSpace1D locomotion includes
#include "Animation/BlendSpace1D.h"
#include "AnimGraphNode_BlendSpacePlayer.h"

FEpicUnrealMCPBlueprintCommands::FEpicUnrealMCPBlueprintCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_blueprint"))
    {
        return HandleCreateBlueprint(Params);
    }
    else if (CommandType == TEXT("add_component_to_blueprint"))
    {
        return HandleAddComponentToBlueprint(Params);
    }
    else if (CommandType == TEXT("set_physics_properties"))
    {
        return HandleSetPhysicsProperties(Params);
    }
    else if (CommandType == TEXT("compile_blueprint"))
    {
        return HandleCompileBlueprint(Params);
    }
    else if (CommandType == TEXT("set_static_mesh_properties"))
    {
        return HandleSetStaticMeshProperties(Params);
    }
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    else if (CommandType == TEXT("set_mesh_material_color"))
    {
        return HandleSetMeshMaterialColor(Params);
    }
    // Material management commands
    else if (CommandType == TEXT("get_available_materials"))
    {
        return HandleGetAvailableMaterials(Params);
    }
    else if (CommandType == TEXT("apply_material_to_actor"))
    {
        return HandleApplyMaterialToActor(Params);
    }
    else if (CommandType == TEXT("apply_material_to_blueprint"))
    {
        return HandleApplyMaterialToBlueprint(Params);
    }
    else if (CommandType == TEXT("get_actor_material_info"))
    {
        return HandleGetActorMaterialInfo(Params);
    }
    else if (CommandType == TEXT("set_mesh_asset_material"))
    {
        return HandleSetMeshAssetMaterial(Params);
    }
    else if (CommandType == TEXT("get_blueprint_material_info"))
    {
        return HandleGetBlueprintMaterialInfo(Params);
    }
    // Blueprint analysis commands
    else if (CommandType == TEXT("read_blueprint_content"))
    {
        return HandleReadBlueprintContent(Params);
    }
    else if (CommandType == TEXT("analyze_blueprint_graph"))
    {
        return HandleAnalyzeBlueprintGraph(Params);
    }
    else if (CommandType == TEXT("get_blueprint_variable_details"))
    {
        return HandleGetBlueprintVariableDetails(Params);
    }
    else if (CommandType == TEXT("get_blueprint_function_details"))
    {
        return HandleGetBlueprintFunctionDetails(Params);
    }
    else if (CommandType == TEXT("create_character_blueprint"))
    {
        return HandleCreateCharacterBlueprint(Params);
    }
    else if (CommandType == TEXT("create_anim_blueprint"))
    {
        return HandleCreateAnimBlueprint(Params);
    }
    else if (CommandType == TEXT("setup_locomotion_state_machine"))
    {
        return HandleSetupLocomotionStateMachine(Params);
    }
    else if (CommandType == TEXT("setup_blendspace_locomotion"))
    {
        return HandleSetupBlendspaceLocomotion(Params);
    }
    else if (CommandType == TEXT("set_character_properties"))
    {
        return HandleSetCharacterProperties(Params);
    }
    else if (CommandType == TEXT("set_anim_sequence_root_motion"))
    {
        return HandleSetAnimSequenceRootMotion(Params);
    }
    else if (CommandType == TEXT("set_anim_state_always_reset_on_entry"))
    {
        return HandleSetAnimStateAlwaysResetOnEntry(Params);
    }
    else if (CommandType == TEXT("set_state_machine_max_transitions_per_frame"))
    {
        return HandleSetStateMachineMaxTransitionsPerFrame(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Check if blueprint already exists (asset registry + in-memory objects)
    FString PackagePath = TEXT("/Game/Blueprints/");
    FString AssetName = BlueprintName;
    FString FullBPPath = PackagePath + AssetName;
    if (UEditorAssetLibrary::DoesAssetExist(FullBPPath))
    {
        TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
        Response->SetStringField(TEXT("name"), AssetName);
        Response->SetStringField(TEXT("path"), FullBPPath);
        Response->SetStringField(TEXT("status"), TEXT("already_exists"));
        return Response;
    }
    // Also check via FindObject (catches in-memory objects the asset registry may miss)
    UPackage* ExistingPkg = FindPackage(nullptr, *FullBPPath);
    if (ExistingPkg && FindObject<UBlueprint>(ExistingPkg, *AssetName))
    {
        TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
        Response->SetStringField(TEXT("name"), AssetName);
        Response->SetStringField(TEXT("path"), FullBPPath);
        Response->SetStringField(TEXT("status"), TEXT("already_exists"));
        return Response;
    }

    // Create the blueprint factory
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    
    // Handle parent class
    FString ParentClass;
    Params->TryGetStringField(TEXT("parent_class"), ParentClass);
    
    // Default to Actor if no parent class specified
    UClass* SelectedParentClass = AActor::StaticClass();
    
    // Try to find the specified parent class
    if (!ParentClass.IsEmpty())
    {
        FString ClassName = ParentClass;
        if (!ClassName.StartsWith(TEXT("A")))
        {
            ClassName = TEXT("A") + ClassName;
        }
        
        // First try direct StaticClass lookup for common classes
        UClass* FoundClass = nullptr;
        if (ClassName == TEXT("APawn"))
        {
            FoundClass = APawn::StaticClass();
        }
        else if (ClassName == TEXT("AActor"))
        {
            FoundClass = AActor::StaticClass();
        }
        else
        {
            // Try loading the class using LoadClass which is more reliable than FindObject
            const FString ClassPath = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
            FoundClass = LoadClass<AActor>(nullptr, *ClassPath);
            
            if (!FoundClass)
            {
                // Try alternate paths if not found
                const FString GameClassPath = FString::Printf(TEXT("/Script/Game.%s"), *ClassName);
                FoundClass = LoadClass<AActor>(nullptr, *GameClassPath);
            }
        }

        if (FoundClass)
        {
            SelectedParentClass = FoundClass;
            UE_LOG(LogTemp, Log, TEXT("Successfully set parent class to '%s'"), *ClassName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Could not find specified parent class '%s' at paths: /Script/Engine.%s or /Script/Game.%s, defaulting to AActor"), 
                *ClassName, *ClassName, *ClassName);
        }
    }
    
    Factory->ParentClass = SelectedParentClass;

    // Create the blueprint
    UPackage* Package = CreatePackage(*(PackagePath + AssetName));
    UBlueprint* NewBlueprint = Cast<UBlueprint>(Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *AssetName, RF_Standalone | RF_Public, nullptr, GWarn));

    if (NewBlueprint)
    {
        // Notify the asset registry
        FAssetRegistryModule::AssetCreated(NewBlueprint);

        // Mark the package dirty
        Package->MarkPackageDirty();

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("name"), AssetName);
        ResultObj->SetStringField(TEXT("path"), PackagePath + AssetName);
        return ResultObj;
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create blueprint"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAddComponentToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentType;
    if (!Params->TryGetStringField(TEXT("component_type"), ComponentType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create the component - dynamically find the component class by name
    UClass* ComponentClass = nullptr;

    // Try to find the class with exact name first
    ComponentClass = FindObject<UClass>(nullptr, *ComponentType);
    
    // If not found, try with "Component" suffix
    if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
    {
        FString ComponentTypeWithSuffix = ComponentType + TEXT("Component");
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithSuffix);
    }
    
    // If still not found, try with "U" prefix
    if (!ComponentClass && !ComponentType.StartsWith(TEXT("U")))
    {
        FString ComponentTypeWithPrefix = TEXT("U") + ComponentType;
        ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithPrefix);
        
        // Try with both prefix and suffix
        if (!ComponentClass && !ComponentType.EndsWith(TEXT("Component")))
        {
            FString ComponentTypeWithBoth = TEXT("U") + ComponentType + TEXT("Component");
            ComponentClass = FindObject<UClass>(nullptr, *ComponentTypeWithBoth);
        }
    }
    
    // Verify that the class is a valid component type
    if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown component type: %s"), *ComponentType));
    }

    // Add the component to the blueprint
    USCS_Node* NewNode = Blueprint->SimpleConstructionScript->CreateNode(ComponentClass, *ComponentName);
    if (NewNode)
    {
        // Set transform if provided
        USceneComponent* SceneComponent = Cast<USceneComponent>(NewNode->ComponentTemplate);
        if (SceneComponent)
        {
            if (Params->HasField(TEXT("location")))
            {
                SceneComponent->SetRelativeLocation(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
            }
            if (Params->HasField(TEXT("rotation")))
            {
                SceneComponent->SetRelativeRotation(FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")));
            }
            if (Params->HasField(TEXT("scale")))
            {
                SceneComponent->SetRelativeScale3D(FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
            }
        }

        // Audio-specific properties for UAudioComponent
        if (UAudioComponent* AudioComp = Cast<UAudioComponent>(NewNode->ComponentTemplate))
        {
            FString SoundPath;
            if (Params->TryGetStringField(TEXT("sound_asset"), SoundPath))
            {
                USoundBase* Sound = Cast<USoundBase>(UEditorAssetLibrary::LoadAsset(SoundPath));
                if (Sound)
                {
                    AudioComp->SetSound(Sound);
                }
            }

            bool bLooping = false;
            if (Params->TryGetBoolField(TEXT("looping"), bLooping) && bLooping)
            {
                // Looping is set on the SoundWave, not the component directly.
                // If the sound asset is a SoundWave, set bLooping on it.
                USoundWave* SoundWave = Cast<USoundWave>(AudioComp->Sound);
                if (SoundWave)
                {
                    SoundWave->bLooping = true;
                }
            }

            bool bAutoActivate = true;
            if (Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate))
            {
                AudioComp->SetAutoActivate(bAutoActivate);
            }

            double VolumeVal = 1.0;
            if (Params->TryGetNumberField(TEXT("volume"), VolumeVal))
            {
                AudioComp->VolumeMultiplier = static_cast<float>(VolumeVal);
            }

            bool bUISound = false;
            if (Params->TryGetBoolField(TEXT("is_ui_sound"), bUISound) && bUISound)
            {
                AudioComp->bIsUISound = true;
                AudioComp->bAllowSpatialization = false;
            }
        }

        // Add to root if no parent specified
        Blueprint->SimpleConstructionScript->AddNode(NewNode);

        // Compile the blueprint
        FKismetEditorUtilities::CompileBlueprint(Blueprint);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("component_name"), ComponentName);
        ResultObj->SetStringField(TEXT("component_type"), ComponentType);
        return ResultObj;
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to add component to blueprint"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetPhysicsProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Set physics properties
    if (Params->HasField(TEXT("simulate_physics")))
    {
        PrimComponent->SetSimulatePhysics(Params->GetBoolField(TEXT("simulate_physics")));
    }

    if (Params->HasField(TEXT("mass")))
    {
        float Mass = Params->GetNumberField(TEXT("mass"));
        // In UE5.5, use proper overrideMass instead of just scaling
        PrimComponent->SetMassOverrideInKg(NAME_None, Mass);
        UE_LOG(LogTemp, Display, TEXT("Set mass for component %s to %f kg"), *ComponentName, Mass);
    }

    if (Params->HasField(TEXT("linear_damping")))
    {
        PrimComponent->SetLinearDamping(Params->GetNumberField(TEXT("linear_damping")));
    }

    if (Params->HasField(TEXT("angular_damping")))
    {
        PrimComponent->SetAngularDamping(Params->GetNumberField(TEXT("angular_damping")));
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Compile with a results log so we can capture errors
    FCompilerResultsLog ResultsLog;
    ResultsLog.bSilentMode = false;
    ResultsLog.bLogInfoOnly = false;
    FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &ResultsLog);

    // Check compilation status
    bool bSuccess = (Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("name"), BlueprintName);
    ResultObj->SetBoolField(TEXT("compiled"), bSuccess);

    // Map status to string
    FString StatusStr;
    switch (Blueprint->Status)
    {
        case BS_UpToDate: StatusStr = TEXT("UpToDate"); break;
        case BS_UpToDateWithWarnings: StatusStr = TEXT("UpToDateWithWarnings"); break;
        case BS_Error: StatusStr = TEXT("Error"); break;
        case BS_Dirty: StatusStr = TEXT("Dirty"); break;
        case BS_Unknown: StatusStr = TEXT("Unknown"); break;
        default: StatusStr = TEXT("Other"); break;
    }
    ResultObj->SetStringField(TEXT("status"), StatusStr);
    ResultObj->SetNumberField(TEXT("num_errors"), ResultsLog.NumErrors);
    ResultObj->SetNumberField(TEXT("num_warnings"), ResultsLog.NumWarnings);

    // Collect error and warning messages
    if (ResultsLog.NumErrors > 0 || ResultsLog.NumWarnings > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ErrorsArray;
        TArray<TSharedPtr<FJsonValue>> WarningsArray;

        for (const TSharedRef<FTokenizedMessage>& Msg : ResultsLog.Messages)
        {
            FString MsgText = Msg->ToText().ToString();
            if (Msg->GetSeverity() == EMessageSeverity::Error)
            {
                ErrorsArray.Add(MakeShared<FJsonValueString>(MsgText));
            }
            else if (Msg->GetSeverity() == EMessageSeverity::Warning || Msg->GetSeverity() == EMessageSeverity::PerformanceWarning)
            {
                WarningsArray.Add(MakeShared<FJsonValueString>(MsgText));
            }
        }

        if (ErrorsArray.Num() > 0)
        {
            ResultObj->SetArrayField(TEXT("errors"), ErrorsArray);
        }
        if (WarningsArray.Num() > 0)
        {
            ResultObj->SetArrayField(TEXT("warnings"), WarningsArray);
        }
    }

    // If compilation failed, also return as an error response format for clarity
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint compilation failed for '%s' with %d error(s)"), *BlueprintName, ResultsLog.NumErrors);
    }

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Starting blueprint actor spawn"));
    
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Missing blueprint_name parameter"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Missing actor_name parameter"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Looking for blueprint '%s'"), *BlueprintName);

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Blueprint not found: %s"), *BlueprintName);
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Blueprint found, getting transform parameters"));

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Location set to (%f, %f, %f)"), Location.X, Location.Y, Location.Z);
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FEpicUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Rotation set to (%f, %f, %f)"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll);
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Getting editor world"));

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Failed to get editor world"));
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Creating spawn transform"));

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));

    // Add a small delay to allow the engine to process the newly compiled class
    FPlatformProcess::Sleep(0.2f);

    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: About to spawn actor from blueprint '%s' with GeneratedClass: %s"), 
           *BlueprintName, Blueprint->GeneratedClass ? *Blueprint->GeneratedClass->GetName() : TEXT("NULL"));

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform);
    
    UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: SpawnActor completed, NewActor: %s"), 
           NewActor ? *NewActor->GetName() : TEXT("NULL"));
    
    if (NewActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: Setting actor label to '%s'"), *ActorName);
        NewActor->SetActorLabel(*ActorName);
        
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: About to convert actor to JSON"));
        TSharedPtr<FJsonObject> Result = FEpicUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
        
        UE_LOG(LogTemp, Warning, TEXT("HandleSpawnBlueprintActor: JSON conversion completed, returning result"));
        return Result;
    }

    UE_LOG(LogTemp, Error, TEXT("HandleSpawnBlueprintActor: Failed to spawn blueprint actor"));
    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetStaticMeshProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Set static mesh properties
    if (Params->HasField(TEXT("static_mesh")))
    {
        FString MeshPath = Params->GetStringField(TEXT("static_mesh"));
        UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
        if (Mesh)
        {
            MeshComponent->SetStaticMesh(Mesh);
        }
    }

    if (Params->HasField(TEXT("material")))
    {
        FString MaterialPath = Params->GetStringField(TEXT("material"));
        UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (Material)
        {
            MeshComponent->SetMaterial(0, Material);
        }
    }

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMeshMaterialColor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    // Try to cast to StaticMeshComponent or PrimitiveComponent
    UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(ComponentNode->ComponentTemplate);
    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a primitive component"));
    }

    // Get color parameter
    TArray<float> ColorArray;
    const TArray<TSharedPtr<FJsonValue>>* ColorJsonArray;
    if (!Params->TryGetArrayField(TEXT("color"), ColorJsonArray) || ColorJsonArray->Num() != 4)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'color' must be an array of 4 float values [R, G, B, A]"));
    }

    for (const TSharedPtr<FJsonValue>& Value : *ColorJsonArray)
    {
        ColorArray.Add(FMath::Clamp(Value->AsNumber(), 0.0f, 1.0f));
    }

    FLinearColor Color(ColorArray[0], ColorArray[1], ColorArray[2], ColorArray[3]);

    // Get material slot index
    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Get parameter name
    FString ParameterName = TEXT("BaseColor");
    Params->TryGetStringField(TEXT("parameter_name"), ParameterName);

    // Get or create material
    UMaterialInterface* Material = nullptr;
    
    // Check if a specific material path was provided
    FString MaterialPath;
    if (Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
        if (!Material)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
        }
    }
    else
    {
        // Use existing material on the component
        Material = PrimComponent->GetMaterial(MaterialSlot);
        if (!Material)
        {
            // Try to use a default material
            Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(TEXT("/Engine/BasicShapes/BasicShapeMaterial")));
            if (!Material)
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No material found on component and failed to load default material"));
            }
        }
    }

    // Create a dynamic material instance
    UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(Material, PrimComponent);
    if (!DynMaterial)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create dynamic material instance"));
    }

    // Set the color parameter
    DynMaterial->SetVectorParameterValue(*ParameterName, Color);

    // Apply the material to the component
    PrimComponent->SetMaterial(MaterialSlot, DynMaterial);

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    // Log success
    UE_LOG(LogTemp, Log, TEXT("Successfully set material color on component %s: R=%f, G=%f, B=%f, A=%f"), 
        *ComponentName, Color.R, Color.G, Color.B, Color.A);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("component"), ComponentName);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetStringField(TEXT("parameter_name"), ParameterName);
    
    TArray<TSharedPtr<FJsonValue>> ColorResultArray;
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.R));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.G));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.B));
    ColorResultArray.Add(MakeShared<FJsonValueNumber>(Color.A));
    ResultObj->SetArrayField(TEXT("color"), ColorResultArray);
    
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetAvailableMaterials(const TSharedPtr<FJsonObject>& Params)
{
    // Get parameters - make search path completely dynamic
    FString SearchPath;
    if (!Params->TryGetStringField(TEXT("search_path"), SearchPath))
    {
        // Default to empty string to search everywhere
        SearchPath = TEXT("");
    }
    
    bool bIncludeEngineMaterials = true;
    if (Params->HasField(TEXT("include_engine_materials")))
    {
        bIncludeEngineMaterials = Params->GetBoolField(TEXT("include_engine_materials"));
    }

    // Get asset registry module
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // Create filter for materials
    FARFilter Filter;
    Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
    Filter.ClassPaths.Add(UMaterialInstanceDynamic::StaticClass()->GetClassPathName());
    
    // Add search paths dynamically
    if (!SearchPath.IsEmpty())
    {
        // Ensure the path starts with /
        if (!SearchPath.StartsWith(TEXT("/")))
        {
            SearchPath = TEXT("/") + SearchPath;
        }
        // Ensure the path ends with / for proper directory search
        if (!SearchPath.EndsWith(TEXT("/")))
        {
            SearchPath += TEXT("/");
        }
        Filter.PackagePaths.Add(*SearchPath);
        UE_LOG(LogTemp, Log, TEXT("Searching for materials in: %s"), *SearchPath);
    }
    else
    {
        // Search in common game content locations
        Filter.PackagePaths.Add(TEXT("/Game/"));
        UE_LOG(LogTemp, Log, TEXT("Searching for materials in all game content"));
    }
    
    if (bIncludeEngineMaterials)
    {
        Filter.PackagePaths.Add(TEXT("/Engine/"));
        UE_LOG(LogTemp, Log, TEXT("Including Engine materials in search"));
    }
    
    Filter.bRecursivePaths = true;

    // Get assets from registry
    TArray<FAssetData> AssetDataArray;
    AssetRegistry.GetAssets(Filter, AssetDataArray);
    
    UE_LOG(LogTemp, Log, TEXT("Asset registry found %d materials"), AssetDataArray.Num());

    // Also try manual search using EditorAssetLibrary for more comprehensive results
    TArray<FString> AllAssetPaths;
    if (!SearchPath.IsEmpty())
    {
        AllAssetPaths = UEditorAssetLibrary::ListAssets(SearchPath, true, false);
    }
    else
    {
        AllAssetPaths = UEditorAssetLibrary::ListAssets(TEXT("/Game/"), true, false);
    }
    
    // Filter for materials from the manual search
    for (const FString& AssetPath : AllAssetPaths)
    {
        if (AssetPath.Contains(TEXT("Material")) && !AssetPath.Contains(TEXT(".uasset")))
        {
            UObject* Asset = UEditorAssetLibrary::LoadAsset(AssetPath);
            if (Asset && Asset->IsA<UMaterialInterface>())
            {
                // Check if we already have this asset from registry search
                bool bAlreadyFound = false;
                for (const FAssetData& ExistingData : AssetDataArray)
                {
                    if (ExistingData.GetObjectPathString() == AssetPath)
                    {
                        bAlreadyFound = true;
                        break;
                    }
                }
                
                if (!bAlreadyFound)
                {
                    // Create FAssetData manually for this asset
                    FAssetData ManualAssetData(Asset);
                    AssetDataArray.Add(ManualAssetData);
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Total materials found after manual search: %d"), AssetDataArray.Num());

    // Convert to JSON
    TArray<TSharedPtr<FJsonValue>> MaterialArray;
    for (const FAssetData& AssetData : AssetDataArray)
    {
        TSharedPtr<FJsonObject> MaterialObj = MakeShared<FJsonObject>();
        MaterialObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
        MaterialObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
        MaterialObj->SetStringField(TEXT("package"), AssetData.PackageName.ToString());
        MaterialObj->SetStringField(TEXT("class"), AssetData.AssetClassPath.ToString());
        
        MaterialArray.Add(MakeShared<FJsonValueObject>(MaterialObj));
        
        UE_LOG(LogTemp, Verbose, TEXT("Found material: %s at %s"), *AssetData.AssetName.ToString(), *AssetData.GetObjectPathString());
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("materials"), MaterialArray);
    ResultObj->SetNumberField(TEXT("count"), MaterialArray.Num());
    ResultObj->SetStringField(TEXT("search_path_used"), SearchPath.IsEmpty() ? TEXT("/Game/") : SearchPath);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleApplyMaterialToActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Find the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    AActor* TargetActor = FEpicUnrealMCPCommonUtils::FindActorByName(World, ActorName);
    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Mark actor for undo and OFPA package dirtying BEFORE mutation
    TargetActor->Modify();

    bool bAppliedToAny = false;
    TSharedPtr<FJsonObject> AppliedMeshes = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> MeshArray;

    // Apply to ALL SkeletalMeshComponents (priority for characters)
    TArray<USkeletalMeshComponent*> SkelMeshComponents;
    TargetActor->GetComponents<USkeletalMeshComponent>(SkelMeshComponents);

    for (USkeletalMeshComponent* SkelComp : SkelMeshComponents)
    {
        if (SkelComp)
        {
            SkelComp->Modify();
            FString CompMeshName;
            if (USkeletalMesh* SkelMesh = SkelComp->GetSkeletalMeshAsset())
            {
                CompMeshName = SkelMesh->GetPathName();
            }
            if (MaterialSlot < 0)
            {
                int32 NumMaterials = SkelComp->GetNumMaterials();
                for (int32 i = 0; i < NumMaterials; i++)
                {
                    SkelComp->SetMaterial(i, Material);
                }
            }
            else
            {
                SkelComp->SetMaterial(MaterialSlot, Material);
            }
            SkelComp->MarkRenderStateDirty();
            bAppliedToAny = true;

            TSharedPtr<FJsonObject> MeshInfo = MakeShared<FJsonObject>();
            MeshInfo->SetStringField(TEXT("mesh"), CompMeshName);
            MeshInfo->SetStringField(TEXT("type"), TEXT("SkeletalMesh"));
            MeshArray.Add(MakeShared<FJsonValueObject>(MeshInfo));
        }
    }

    // Also apply to StaticMeshComponents
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    TargetActor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);

    for (UStaticMeshComponent* MeshComp : StaticMeshComponents)
    {
        if (MeshComp)
        {
            MeshComp->Modify();
            FString CompMeshName;
            if (UStaticMesh* Mesh = MeshComp->GetStaticMesh())
            {
                CompMeshName = Mesh->GetPathName();
            }
            if (MaterialSlot < 0)
            {
                int32 NumMaterials = MeshComp->GetNumMaterials();
                for (int32 i = 0; i < NumMaterials; i++)
                {
                    MeshComp->SetMaterial(i, Material);
                }
            }
            else
            {
                MeshComp->SetMaterial(MaterialSlot, Material);
            }
            MeshComp->MarkRenderStateDirty();
            bAppliedToAny = true;

            TSharedPtr<FJsonObject> MeshInfo = MakeShared<FJsonObject>();
            MeshInfo->SetStringField(TEXT("mesh"), CompMeshName);
            MeshInfo->SetStringField(TEXT("type"), TEXT("StaticMesh"));
            MeshArray.Add(MakeShared<FJsonValueObject>(MeshInfo));
        }
    }

    if (!bAppliedToAny)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No mesh components found on actor (checked StaticMesh and SkeletalMesh)"));
    }

    // Mark the actor's package dirty so OFPA serializes the material override
    TargetActor->MarkPackageDirty();

    // Also dirty the OFPA external package for proper persistence
    if (UPackage* ExternalPackage = TargetActor->GetExternalPackage())
    {
        ExternalPackage->SetDirtyFlag(true);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor_name"), ActorName);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetArrayField(TEXT("applied_to"), MeshArray);
    ResultObj->SetBoolField(TEXT("success"), true);

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetMeshAssetMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString MeshPath;
    if (!Params->TryGetStringField(TEXT("mesh_path"), MeshPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'mesh_path' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Try loading as StaticMesh first, then SkeletalMesh
    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(MeshPath);
    if (!LoadedAsset)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load asset: %s"), *MeshPath));
    }

    int32 TotalSlots = 0;
    FString MeshType;

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(LoadedAsset))
    {
        MeshType = TEXT("StaticMesh");
        TArray<FStaticMaterial>& StaticMaterials = StaticMesh->GetStaticMaterials();
        TotalSlots = StaticMaterials.Num();

        if (MaterialSlot < 0)
        {
            StaticMesh->Modify();
            for (int32 i = 0; i < StaticMaterials.Num(); i++)
            {
                StaticMaterials[i].MaterialInterface = Material;
            }
        }
        else
        {
            if (MaterialSlot >= StaticMaterials.Num())
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material slot %d out of range (mesh has %d slots)"), MaterialSlot, StaticMaterials.Num()));
            }
            StaticMesh->Modify();
            StaticMaterials[MaterialSlot].MaterialInterface = Material;
        }

        StaticMesh->PostEditChange();
        StaticMesh->MarkPackageDirty();
        UEditorAssetLibrary::SaveLoadedAsset(StaticMesh);
    }
    else if (USkeletalMesh* SkelMesh = Cast<USkeletalMesh>(LoadedAsset))
    {
        MeshType = TEXT("SkeletalMesh");
        TArray<FSkeletalMaterial>& SkelMaterials = SkelMesh->GetMaterials();
        TotalSlots = SkelMaterials.Num();

        if (MaterialSlot < 0)
        {
            SkelMesh->Modify();
            for (int32 i = 0; i < SkelMaterials.Num(); i++)
            {
                SkelMaterials[i].MaterialInterface = Material;
            }
        }
        else
        {
            if (MaterialSlot >= SkelMaterials.Num())
            {
                return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material slot %d out of range (mesh has %d slots)"), MaterialSlot, SkelMaterials.Num()));
            }
            SkelMesh->Modify();
            SkelMaterials[MaterialSlot].MaterialInterface = Material;
        }

        SkelMesh->PostEditChange();
        SkelMesh->MarkPackageDirty();
        UEditorAssetLibrary::SaveLoadedAsset(SkelMesh);
    }
    else
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset is not a StaticMesh or SkeletalMesh: %s"), *MeshPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("mesh_path"), MeshPath);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetStringField(TEXT("mesh_type"), MeshType);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetNumberField(TEXT("total_slots"), TotalSlots);
    ResultObj->SetBoolField(TEXT("success"), true);

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleApplyMaterialToBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }

    int32 MaterialSlot = 0;
    if (Params->HasField(TEXT("material_slot")))
    {
        MaterialSlot = Params->GetIntegerField(TEXT("material_slot"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component - try SCS first, then CDO for inherited components
    UPrimitiveComponent* PrimComponent = nullptr;

    // Pass 1: Search SCS nodes (user-added components)
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (Node && Node->GetVariableName().ToString() == ComponentName)
            {
                PrimComponent = Cast<UPrimitiveComponent>(Node->ComponentTemplate);
                break;
            }
        }
    }

    // Pass 2: Search CDO components (inherited from C++ parent, e.g. ACharacter::Mesh)
    if (!PrimComponent && Blueprint->GeneratedClass)
    {
        AActor* CDO = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
        if (CDO)
        {
            TArray<UActorComponent*> AllComps;
            CDO->GetComponents(AllComps);
            for (UActorComponent* Comp : AllComps)
            {
                UPrimitiveComponent* PC = Cast<UPrimitiveComponent>(Comp);
                if (PC && PC->GetName() == ComponentName)
                {
                    PrimComponent = PC;
                    break;
                }
            }
        }
    }

    if (!PrimComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("Component '%s' not found in SCS or CDO of blueprint '%s'"), *ComponentName, *BlueprintName));
    }

    // Load the material
    UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
    if (!Material)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load material: %s"), *MaterialPath));
    }

    // Apply the material
    PrimComponent->Modify();
    if (MaterialSlot < 0)
    {
        int32 NumMaterials = PrimComponent->GetNumMaterials();
        for (int32 i = 0; i < NumMaterials; i++)
        {
            PrimComponent->SetMaterial(i, Material);
        }
    }
    else
    {
        if (MaterialSlot >= PrimComponent->GetNumMaterials())
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                TEXT("Material slot %d out of range (component has %d slots)"), MaterialSlot, PrimComponent->GetNumMaterials()));
        }
        PrimComponent->SetMaterial(MaterialSlot, Material);
    }
    PrimComponent->MarkRenderStateDirty();

    // Mark the blueprint as modified
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetStringField(TEXT("material_path"), MaterialPath);
    ResultObj->SetNumberField(TEXT("material_slot"), MaterialSlot);
    ResultObj->SetBoolField(TEXT("success"), true);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    AActor* TargetActor = FEpicUnrealMCPCommonUtils::FindActorByName(World, ActorName);
    if (!TargetActor)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get mesh components and their materials
    TArray<UStaticMeshComponent*> MeshComponents;
    TargetActor->GetComponents<UStaticMeshComponent>(MeshComponents);

    TArray<TSharedPtr<FJsonValue>> MaterialSlots;

    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp)
        {
            for (int32 i = 0; i < MeshComp->GetNumMaterials(); i++)
            {
                TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
                SlotInfo->SetNumberField(TEXT("slot"), i);
                SlotInfo->SetStringField(TEXT("component"), MeshComp->GetName());
                SlotInfo->SetStringField(TEXT("component_type"), TEXT("StaticMesh"));

                UMaterialInterface* Material = MeshComp->GetMaterial(i);
                if (Material)
                {
                    SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                    SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                    SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
                }
                else
                {
                    SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                    SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                    SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
                }

                MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
            }
        }
    }

    // Also check SkeletalMeshComponents
    TArray<USkeletalMeshComponent*> SkelMeshComponents;
    TargetActor->GetComponents<USkeletalMeshComponent>(SkelMeshComponents);

    for (USkeletalMeshComponent* SkelComp : SkelMeshComponents)
    {
        if (SkelComp)
        {
            for (int32 i = 0; i < SkelComp->GetNumMaterials(); i++)
            {
                TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
                SlotInfo->SetNumberField(TEXT("slot"), i);
                SlotInfo->SetStringField(TEXT("component"), SkelComp->GetName());
                SlotInfo->SetStringField(TEXT("component_type"), TEXT("SkeletalMesh"));

                UMaterialInterface* Material = SkelComp->GetMaterial(i);
                if (Material)
                {
                    SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                    SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                    SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
                }
                else
                {
                    SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                    SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                    SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
                }

                MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
            }
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor_name"), ActorName);
    ResultObj->SetArrayField(TEXT("material_slots"), MaterialSlots);
    ResultObj->SetNumberField(TEXT("total_slots"), MaterialSlots.Num());
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FEpicUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the component
    USCS_Node* ComponentNode = nullptr;
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        if (Node && Node->GetVariableName().ToString() == ComponentName)
        {
            ComponentNode = Node;
            break;
        }
    }

    if (!ComponentNode)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Component not found: %s"), *ComponentName));
    }

    UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ComponentNode->ComponentTemplate);
    if (!MeshComponent)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Component is not a static mesh component"));
    }

    // Get material slot information
    TArray<TSharedPtr<FJsonValue>> MaterialSlots;
    int32 NumMaterials = 0;
    
    // Check if we have a static mesh assigned
    UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
    if (StaticMesh)
    {
        NumMaterials = StaticMesh->GetNumSections(0); // Get number of material slots for LOD 0
        
        for (int32 i = 0; i < NumMaterials; i++)
        {
            TSharedPtr<FJsonObject> SlotInfo = MakeShared<FJsonObject>();
            SlotInfo->SetNumberField(TEXT("slot"), i);
            SlotInfo->SetStringField(TEXT("component"), ComponentName);
            
            UMaterialInterface* Material = MeshComponent->GetMaterial(i);
            if (Material)
            {
                SlotInfo->SetStringField(TEXT("material_name"), Material->GetName());
                SlotInfo->SetStringField(TEXT("material_path"), Material->GetPathName());
                SlotInfo->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());
            }
            else
            {
                SlotInfo->SetStringField(TEXT("material_name"), TEXT("None"));
                SlotInfo->SetStringField(TEXT("material_path"), TEXT(""));
                SlotInfo->SetStringField(TEXT("material_class"), TEXT(""));
            }
            
            MaterialSlots.Add(MakeShared<FJsonValueObject>(SlotInfo));
        }
    }
    else
    {
        // If no static mesh is assigned, we can't determine material slots
        UE_LOG(LogTemp, Warning, TEXT("No static mesh assigned to component %s in blueprint %s"), *ComponentName, *BlueprintName);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_name"), BlueprintName);
    ResultObj->SetStringField(TEXT("component_name"), ComponentName);
    ResultObj->SetArrayField(TEXT("material_slots"), MaterialSlots);
    ResultObj->SetNumberField(TEXT("total_slots"), MaterialSlots.Num());
    ResultObj->SetBoolField(TEXT("has_static_mesh"), StaticMesh != nullptr);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleReadBlueprintContent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    // Get optional parameters
    bool bIncludeEventGraph = true;
    bool bIncludeFunctions = true;
    bool bIncludeVariables = true;
    bool bIncludeComponents = true;
    bool bIncludeInterfaces = true;

    Params->TryGetBoolField(TEXT("include_event_graph"), bIncludeEventGraph);
    Params->TryGetBoolField(TEXT("include_functions"), bIncludeFunctions);
    Params->TryGetBoolField(TEXT("include_variables"), bIncludeVariables);
    Params->TryGetBoolField(TEXT("include_components"), bIncludeComponents);
    Params->TryGetBoolField(TEXT("include_interfaces"), bIncludeInterfaces);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    ResultObj->SetStringField(TEXT("blueprint_name"), Blueprint->GetName());
    ResultObj->SetStringField(TEXT("parent_class"), Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"));

    // Include variables if requested
    if (bIncludeVariables)
    {
        TArray<TSharedPtr<FJsonValue>> VariableArray;
        for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
        {
            TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
            VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
            VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
            VarObj->SetStringField(TEXT("default_value"), Variable.DefaultValue);
            VarObj->SetBoolField(TEXT("is_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
            VariableArray.Add(MakeShared<FJsonValueObject>(VarObj));
        }
        ResultObj->SetArrayField(TEXT("variables"), VariableArray);
    }

    // Include functions if requested
    if (bIncludeFunctions)
    {
        TArray<TSharedPtr<FJsonValue>> FunctionArray;
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph)
            {
                TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
                FuncObj->SetStringField(TEXT("name"), Graph->GetName());
                FuncObj->SetStringField(TEXT("graph_type"), TEXT("Function"));
                
                // Count nodes in function
                int32 NodeCount = Graph->Nodes.Num();
                FuncObj->SetNumberField(TEXT("node_count"), NodeCount);
                
                FunctionArray.Add(MakeShared<FJsonValueObject>(FuncObj));
            }
        }
        ResultObj->SetArrayField(TEXT("functions"), FunctionArray);
    }

    // Include event graph if requested
    if (bIncludeEventGraph)
    {
        TSharedPtr<FJsonObject> EventGraphObj = MakeShared<FJsonObject>();
        
        // Find the main event graph
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph && Graph->GetName() == TEXT("EventGraph"))
            {
                EventGraphObj->SetStringField(TEXT("name"), Graph->GetName());
                EventGraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
                
                // Get basic node information
                TArray<TSharedPtr<FJsonValue>> NodeArray;
                for (UEdGraphNode* Node : Graph->Nodes)
                {
                    if (Node)
                    {
                        TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                        NodeObj->SetStringField(TEXT("name"), Node->GetName());
                        NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
                        NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                        NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
                    }
                }
                EventGraphObj->SetArrayField(TEXT("nodes"), NodeArray);
                break;
            }
        }
        
        ResultObj->SetObjectField(TEXT("event_graph"), EventGraphObj);
    }

    // Include components if requested
    if (bIncludeComponents)
    {
        TArray<TSharedPtr<FJsonValue>> ComponentArray;
        if (Blueprint->SimpleConstructionScript)
        {
            for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
            {
                if (Node && Node->ComponentTemplate)
                {
                    TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
                    CompObj->SetStringField(TEXT("name"), Node->GetVariableName().ToString());
                    CompObj->SetStringField(TEXT("class"), Node->ComponentTemplate->GetClass()->GetName());
                    CompObj->SetBoolField(TEXT("is_root"), Node == Blueprint->SimpleConstructionScript->GetDefaultSceneRootNode());
                    ComponentArray.Add(MakeShared<FJsonValueObject>(CompObj));
                }
            }
        }
        ResultObj->SetArrayField(TEXT("components"), ComponentArray);
    }

    // Include interfaces if requested
    if (bIncludeInterfaces)
    {
        TArray<TSharedPtr<FJsonValue>> InterfaceArray;
        for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
        {
            TSharedPtr<FJsonObject> InterfaceObj = MakeShared<FJsonObject>();
            InterfaceObj->SetStringField(TEXT("name"), Interface.Interface ? Interface.Interface->GetName() : TEXT("Unknown"));
            InterfaceArray.Add(MakeShared<FJsonValueObject>(InterfaceObj));
        }
        ResultObj->SetArrayField(TEXT("interfaces"), InterfaceArray);
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleAnalyzeBlueprintGraph(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString GraphName = TEXT("EventGraph");
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    // Get optional parameters
    bool bIncludeNodeDetails = true;
    bool bIncludePinConnections = true;
    bool bTraceExecutionFlow = true;

    Params->TryGetBoolField(TEXT("include_node_details"), bIncludeNodeDetails);
    Params->TryGetBoolField(TEXT("include_pin_connections"), bIncludePinConnections);
    Params->TryGetBoolField(TEXT("trace_execution_flow"), bTraceExecutionFlow);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    // Find the specified graph
    UEdGraph* TargetGraph = nullptr;
    
    // Check event graphs first
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph && Graph->GetName() == GraphName)
        {
            TargetGraph = Graph;
            break;
        }
    }
    
    // Check function graphs if not found
    if (!TargetGraph)
    {
        for (UEdGraph* Graph : Blueprint->FunctionGraphs)
        {
            if (Graph && Graph->GetName() == GraphName)
            {
                TargetGraph = Graph;
                break;
            }
        }
    }

    if (!TargetGraph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    TSharedPtr<FJsonObject> GraphData = MakeShared<FJsonObject>();
    GraphData->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
    GraphData->SetStringField(TEXT("graph_type"), TargetGraph->GetClass()->GetName());

    // Analyze nodes
    TArray<TSharedPtr<FJsonValue>> NodeArray;
    TArray<TSharedPtr<FJsonValue>> ConnectionArray;

    for (UEdGraphNode* Node : TargetGraph->Nodes)
    {
        if (Node)
        {
            TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
            NodeObj->SetStringField(TEXT("name"), Node->GetName());
            NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
            NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

            if (bIncludeNodeDetails)
            {
                NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
                NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);
                NodeObj->SetBoolField(TEXT("can_rename"), Node->bCanRenameNode);
            }

            // Include pin information if requested
            if (bIncludePinConnections)
            {
                TArray<TSharedPtr<FJsonValue>> PinArray;
                for (UEdGraphPin* Pin : Node->Pins)
                {
                    if (Pin)
                    {
                        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                        PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
                        PinObj->SetNumberField(TEXT("connections"), Pin->LinkedTo.Num());
                        
                        // Record connections for this pin
                        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                        {
                            if (LinkedPin && LinkedPin->GetOwningNode())
                            {
                                TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                                ConnObj->SetStringField(TEXT("from_node"), Pin->GetOwningNode()->GetName());
                                ConnObj->SetStringField(TEXT("from_pin"), Pin->PinName.ToString());
                                ConnObj->SetStringField(TEXT("to_node"), LinkedPin->GetOwningNode()->GetName());
                                ConnObj->SetStringField(TEXT("to_pin"), LinkedPin->PinName.ToString());
                                ConnectionArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                            }
                        }
                        
                        PinArray.Add(MakeShared<FJsonValueObject>(PinObj));
                    }
                }
                NodeObj->SetArrayField(TEXT("pins"), PinArray);
            }

            NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
        }
    }

    GraphData->SetArrayField(TEXT("nodes"), NodeArray);
    GraphData->SetArrayField(TEXT("connections"), ConnectionArray);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    ResultObj->SetObjectField(TEXT("graph_data"), GraphData);
    ResultObj->SetBoolField(TEXT("success"), true);

    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintVariableDetails(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString VariableName;
    bool bSpecificVariable = Params->TryGetStringField(TEXT("variable_name"), VariableName);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> VariableArray;

    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        // If looking for specific variable, skip others
        if (bSpecificVariable && Variable.VarName.ToString() != VariableName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
        VarObj->SetStringField(TEXT("name"), Variable.VarName.ToString());
        VarObj->SetStringField(TEXT("type"), Variable.VarType.PinCategory.ToString());
        VarObj->SetStringField(TEXT("sub_category"), Variable.VarType.PinSubCategory.ToString());
        VarObj->SetStringField(TEXT("default_value"), Variable.DefaultValue);
        VarObj->SetStringField(TEXT("friendly_name"), Variable.FriendlyName.IsEmpty() ? Variable.VarName.ToString() : Variable.FriendlyName);
        
        // Get tooltip from metadata (VarTooltip doesn't exist in UE 5.5)
        FString TooltipValue;
        if (Variable.HasMetaData(FBlueprintMetadata::MD_Tooltip))
        {
            TooltipValue = Variable.GetMetaData(FBlueprintMetadata::MD_Tooltip);
        }
        VarObj->SetStringField(TEXT("tooltip"), TooltipValue);
        
        VarObj->SetStringField(TEXT("category"), Variable.Category.ToString());

        // Property flags
        VarObj->SetBoolField(TEXT("is_editable"), (Variable.PropertyFlags & CPF_Edit) != 0);
        VarObj->SetBoolField(TEXT("is_blueprint_visible"), (Variable.PropertyFlags & CPF_BlueprintVisible) != 0);
        VarObj->SetBoolField(TEXT("is_editable_in_instance"), (Variable.PropertyFlags & CPF_DisableEditOnInstance) == 0);
        VarObj->SetBoolField(TEXT("is_config"), (Variable.PropertyFlags & CPF_Config) != 0);

        // Replication
        VarObj->SetNumberField(TEXT("replication"), (int32)Variable.ReplicationCondition);

        VariableArray.Add(MakeShared<FJsonValueObject>(VarObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    
    if (bSpecificVariable)
    {
        ResultObj->SetStringField(TEXT("variable_name"), VariableName);
        if (VariableArray.Num() > 0)
        {
            ResultObj->SetObjectField(TEXT("variable"), VariableArray[0]->AsObject());
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
        }
    }
    else
    {
        ResultObj->SetArrayField(TEXT("variables"), VariableArray);
        ResultObj->SetNumberField(TEXT("variable_count"), VariableArray.Num());
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleGetBlueprintFunctionDetails(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintPath;
    if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
    }

    FString FunctionName;
    bool bSpecificFunction = Params->TryGetStringField(TEXT("function_name"), FunctionName);

    bool bIncludeGraph = true;
    Params->TryGetBoolField(TEXT("include_graph"), bIncludeGraph);

    // Load the blueprint
    UBlueprint* Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load blueprint: %s"), *BlueprintPath));
    }

    TArray<TSharedPtr<FJsonValue>> FunctionArray;

    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (!Graph) continue;

        // If looking for specific function, skip others
        if (bSpecificFunction && Graph->GetName() != FunctionName)
        {
            continue;
        }

        TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
        FuncObj->SetStringField(TEXT("name"), Graph->GetName());
        FuncObj->SetStringField(TEXT("graph_type"), TEXT("Function"));

        // Get function signature from graph
        TArray<TSharedPtr<FJsonValue>> InputPins;
        TArray<TSharedPtr<FJsonValue>> OutputPins;

        // Find function entry and result nodes
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node)
            {
                if (Node->GetClass()->GetName().Contains(TEXT("FunctionEntry")))
                {
                    // Process input parameters
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Output && Pin->PinName != TEXT("then"))
                        {
                            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                            InputPins.Add(MakeShared<FJsonValueObject>(PinObj));
                        }
                    }
                }
                else if (Node->GetClass()->GetName().Contains(TEXT("FunctionResult")))
                {
                    // Process output parameters
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != TEXT("exec"))
                        {
                            TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
                            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                            OutputPins.Add(MakeShared<FJsonValueObject>(PinObj));
                        }
                    }
                }
            }
        }

        FuncObj->SetArrayField(TEXT("input_parameters"), InputPins);
        FuncObj->SetArrayField(TEXT("output_parameters"), OutputPins);
        FuncObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

        // Include graph details if requested
        if (bIncludeGraph)
        {
            TArray<TSharedPtr<FJsonValue>> NodeArray;
            for (UEdGraphNode* Node : Graph->Nodes)
            {
                if (Node)
                {
                    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
                    NodeObj->SetStringField(TEXT("name"), Node->GetName());
                    NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
                    NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
                    NodeArray.Add(MakeShared<FJsonValueObject>(NodeObj));
                }
            }
            FuncObj->SetArrayField(TEXT("graph_nodes"), NodeArray);
        }

        FunctionArray.Add(MakeShared<FJsonValueObject>(FuncObj));
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("blueprint_path"), BlueprintPath);
    
    if (bSpecificFunction)
    {
        ResultObj->SetStringField(TEXT("function_name"), FunctionName);
        if (FunctionArray.Num() > 0)
        {
            ResultObj->SetObjectField(TEXT("function"), FunctionArray[0]->AsObject());
        }
        else
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s"), *FunctionName));
        }
    }
    else
    {
        ResultObj->SetArrayField(TEXT("functions"), FunctionArray);
        ResultObj->SetNumberField(TEXT("function_count"), FunctionArray.Num());
    }

    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateCharacterBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString BlueprintPath = TEXT("/Game/Characters/");
    Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);
    if (!BlueprintPath.EndsWith(TEXT("/")))
    {
        BlueprintPath += TEXT("/");
    }

    FString FullPath = BlueprintPath + BlueprintName;

    // Check if already exists
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Blueprint already exists: %s"), *FullPath));
    }

    // Get optional parameters
    FString SkeletalMeshPath;
    Params->TryGetStringField(TEXT("skeletal_mesh_path"), SkeletalMeshPath);

    FString AnimBlueprintPath;
    Params->TryGetStringField(TEXT("anim_blueprint_path"), AnimBlueprintPath);

    double CapsuleRadius = 40.0;
    Params->TryGetNumberField(TEXT("capsule_radius"), CapsuleRadius);

    double CapsuleHalfHeight = 90.0;
    Params->TryGetNumberField(TEXT("capsule_half_height"), CapsuleHalfHeight);

    double MaxWalkSpeed = 500.0;
    Params->TryGetNumberField(TEXT("max_walk_speed"), MaxWalkSpeed);

    double MaxSprintSpeed = 800.0;
    Params->TryGetNumberField(TEXT("max_sprint_speed"), MaxSprintSpeed);

    double JumpZVelocity = 420.0;
    Params->TryGetNumberField(TEXT("jump_z_velocity"), JumpZVelocity);

    double CameraBoomLength = 250.0;
    Params->TryGetNumberField(TEXT("camera_boom_length"), CameraBoomLength);

    double CameraBoomOffsetZ = 150.0;
    Params->TryGetNumberField(TEXT("camera_boom_socket_offset_z"), CameraBoomOffsetZ);

    // ---- Phase 1: Create Blueprint with ACharacter parent ----
    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    Factory->ParentClass = ACharacter::StaticClass();

    UPackage* Package = CreatePackage(*FullPath);
    UBlueprint* NewBP = Cast<UBlueprint>(
        Factory->FactoryCreateNew(UBlueprint::StaticClass(), Package, *BlueprintName,
            RF_Standalone | RF_Public, nullptr, GWarn));

    if (!NewBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Character Blueprint"));
    }

    FAssetRegistryModule::AssetCreated(NewBP);
    Package->MarkPackageDirty();

    // ---- Phase 2: First compile to create CDO ----
    FKismetEditorUtilities::CompileBlueprint(NewBP);

    // ---- Phase 3: Configure CDO components (inherited from ACharacter) ----
    ACharacter* CDO = NewBP->GeneratedClass->GetDefaultObject<ACharacter>();
    if (!CDO)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get Character CDO after compile"));
    }

    // Configure capsule
    if (UCapsuleComponent* Capsule = CDO->GetCapsuleComponent())
    {
        Capsule->SetCapsuleRadius(CapsuleRadius);
        Capsule->SetCapsuleHalfHeight(CapsuleHalfHeight);
    }

    // Configure character movement
    if (UCharacterMovementComponent* Movement = CDO->GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = MaxWalkSpeed;
        Movement->JumpZVelocity = JumpZVelocity;
        Movement->bOrientRotationToMovement = true;
        Movement->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
        Movement->AirControl = 0.35f;
        Movement->BrakingDecelerationWalking = 2000.0f;
    }

    // Don't use controller rotation for character mesh
    CDO->bUseControllerRotationPitch = false;
    CDO->bUseControllerRotationYaw = false;
    CDO->bUseControllerRotationRoll = false;

    // Configure skeletal mesh if provided
    if (!SkeletalMeshPath.IsEmpty())
    {
        USkeletalMesh* SkelMesh = LoadObject<USkeletalMesh>(nullptr, *SkeletalMeshPath);
        if (SkelMesh)
        {
            if (USkeletalMeshComponent* MeshComp = CDO->GetMesh())
            {
                MeshComp->SetSkeletalMesh(SkelMesh);
                MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleHalfHeight));
                MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f)); // Face forward

                // Set animation blueprint if provided
                if (!AnimBlueprintPath.IsEmpty())
                {
                    UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
                    if (AnimBP && AnimBP->GeneratedClass)
                    {
                        MeshComp->SetAnimInstanceClass(AnimBP->GeneratedClass);
                    }
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("create_character_blueprint: Could not load skeletal mesh at '%s'"), *SkeletalMeshPath);
        }
    }

    // ---- Phase 4: Add SpringArm + Camera via SCS (not inherited from ACharacter) ----
    USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
    if (SCS)
    {
        // Create SpringArm (CameraBoom)
        USCS_Node* SpringArmNode = SCS->CreateNode(USpringArmComponent::StaticClass(), TEXT("CameraBoom"));
        if (SpringArmNode)
        {
            USpringArmComponent* SpringArmTemplate = Cast<USpringArmComponent>(SpringArmNode->ComponentTemplate);
            if (SpringArmTemplate)
            {
                SpringArmTemplate->TargetArmLength = CameraBoomLength;
                SpringArmTemplate->SocketOffset = FVector(0.0f, 0.0f, CameraBoomOffsetZ);
                SpringArmTemplate->bUsePawnControlRotation = true;
                SpringArmTemplate->bEnableCameraLag = true;
                SpringArmTemplate->CameraLagSpeed = 10.0f;
            }

            // Attach to root (capsule)
            SCS->AddNode(SpringArmNode);

            // Create Camera attached to SpringArm
            USCS_Node* CameraNode = SCS->CreateNode(UCameraComponent::StaticClass(), TEXT("FollowCamera"));
            if (CameraNode)
            {
                SpringArmNode->AddChildNode(CameraNode);
            }
        }
    }

    // ---- Phase 5: Final compile and save ----
    FKismetEditorUtilities::CompileBlueprint(NewBP);
    Package->MarkPackageDirty();

    // Save the blueprint package
    FString PackageFilename = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewBP, *PackageFilename, SaveArgs);

    // ---- Build response ----
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), BlueprintName);
    Result->SetStringField(TEXT("path"), FullPath);
    Result->SetStringField(TEXT("parent_class"), TEXT("Character"));

    if (NewBP->GeneratedClass)
    {
        Result->SetStringField(TEXT("generated_class"), NewBP->GeneratedClass->GetPathName());
    }

    // List components
    TArray<TSharedPtr<FJsonValue>> ComponentList;

    auto AddComponent = [&ComponentList](const FString& Name, const FString& Type) {
        TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
        CompObj->SetStringField(TEXT("name"), Name);
        CompObj->SetStringField(TEXT("type"), Type);
        ComponentList.Add(MakeShared<FJsonValueObject>(CompObj));
    };

    AddComponent(TEXT("CapsuleComponent"), TEXT("UCapsuleComponent"));
    AddComponent(TEXT("Mesh"), TEXT("USkeletalMeshComponent"));
    AddComponent(TEXT("CharacterMovement"), TEXT("UCharacterMovementComponent"));
    AddComponent(TEXT("CameraBoom"), TEXT("USpringArmComponent"));
    AddComponent(TEXT("FollowCamera"), TEXT("UCameraComponent"));

    Result->SetArrayField(TEXT("components"), ComponentList);

    // Settings summary
    TSharedPtr<FJsonObject> Settings = MakeShared<FJsonObject>();
    Settings->SetNumberField(TEXT("capsule_radius"), CapsuleRadius);
    Settings->SetNumberField(TEXT("capsule_half_height"), CapsuleHalfHeight);
    Settings->SetNumberField(TEXT("max_walk_speed"), MaxWalkSpeed);
    Settings->SetNumberField(TEXT("jump_z_velocity"), JumpZVelocity);
    Settings->SetNumberField(TEXT("camera_boom_length"), CameraBoomLength);
    Settings->SetNumberField(TEXT("camera_boom_socket_offset_z"), CameraBoomOffsetZ);
    Result->SetObjectField(TEXT("settings"), Settings);

    Result->SetStringField(TEXT("message"), TEXT("Character Blueprint created successfully"));
    return Result;
}
TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Skeleton is REQUIRED for AnimBlueprint (crash without it)
    FString SkeletonPath;
    if (!Params->TryGetStringField(TEXT("skeleton_path"), SkeletonPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Missing 'skeleton_path' parameter. AnimBlueprint requires a target skeleton."));
    }

    USkeleton* TargetSkeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath);
    if (!TargetSkeleton)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Could not load skeleton at: %s"), *SkeletonPath));
    }

    FString BlueprintPath = TEXT("/Game/Characters/");
    Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath);
    if (!BlueprintPath.EndsWith(TEXT("/")))
    {
        BlueprintPath += TEXT("/");
    }

    FString FullPath = BlueprintPath + BlueprintName;

    // Check if already exists — check both asset registry AND loaded objects in memory
    // FindObject catches cases where package is loaded but asset registry is stale
    if (UEditorAssetLibrary::DoesAssetExist(FullPath))
    {
        // Asset exists — load and return it instead of crashing
        UAnimBlueprint* ExistingBP = LoadObject<UAnimBlueprint>(nullptr, *FullPath);
        if (ExistingBP)
        {
            TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
            Response->SetStringField(TEXT("blueprint_path"), FullPath);
            Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
            Response->SetStringField(TEXT("status"), TEXT("already_exists"));
            return Response;
        }
    }

    // Also check via FindObject (catches in-memory objects the asset registry may miss)
    UPackage* ExistingPackage = FindPackage(nullptr, *FullPath);
    if (ExistingPackage)
    {
        UBlueprint* ExistingBP = FindObject<UBlueprint>(ExistingPackage, *BlueprintName);
        if (ExistingBP)
        {
            TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
            Response->SetStringField(TEXT("blueprint_path"), FullPath);
            Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
            Response->SetStringField(TEXT("status"), TEXT("already_exists"));
            return Response;
        }
    }

    // Optional: preview mesh for the anim editor
    FString PreviewMeshPath;
    Params->TryGetStringField(TEXT("preview_mesh_path"), PreviewMeshPath);

    // Create the AnimBlueprint using UAnimBlueprintFactory
    UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
    Factory->TargetSkeleton = TargetSkeleton;
    Factory->ParentClass = UAnimInstance::StaticClass();

    UPackage* Package = CreatePackage(*FullPath);

    // Final safety: check if the name is taken in the newly created/found package
    if (FindObject<UBlueprint>(Package, *BlueprintName))
    {
        TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
        Response->SetStringField(TEXT("blueprint_path"), FullPath);
        Response->SetStringField(TEXT("blueprint_name"), BlueprintName);
        Response->SetStringField(TEXT("status"), TEXT("already_exists"));
        return Response;
    }

    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(
        Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(), Package, *BlueprintName,
            RF_Standalone | RF_Public, nullptr, GWarn));

    if (!AnimBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create AnimBlueprint"));
    }

    // Set preview mesh if provided
    if (!PreviewMeshPath.IsEmpty())
    {
        USkeletalMesh* PreviewMesh = LoadObject<USkeletalMesh>(nullptr, *PreviewMeshPath);
        if (PreviewMesh)
        {
            AnimBP->SetPreviewMesh(PreviewMesh);
        }
    }

    // Register and save
    FAssetRegistryModule::AssetCreated(AnimBP);
    Package->MarkPackageDirty();

    // Compile the AnimBlueprint
    FKismetEditorUtilities::CompileBlueprint(AnimBP);

    // Save package
    FString PackageFilename = FPackageName::LongPackageNameToFilename(FullPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, AnimBP, *PackageFilename, SaveArgs);

    // Build response
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("name"), BlueprintName);
    Result->SetStringField(TEXT("path"), FullPath);
    Result->SetStringField(TEXT("skeleton_path"), TargetSkeleton->GetPathName());
    Result->SetStringField(TEXT("parent_class"), TEXT("AnimInstance"));

    if (AnimBP->GeneratedClass)
    {
        Result->SetStringField(TEXT("generated_class"), AnimBP->GeneratedClass->GetPathName());
    }

    Result->SetStringField(TEXT("message"), TEXT("AnimBlueprint created successfully"));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetupLocomotionStateMachine(const TSharedPtr<FJsonObject>& Params)
{
	// === Part 0: Parse parameters ===
	FString AnimBPPath;
	if (!Params->TryGetStringField(TEXT("anim_blueprint_path"), AnimBPPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint_path'"));
	}

	FString IdleAnimPath, WalkAnimPath, RunAnimPath;
	if (!Params->TryGetStringField(TEXT("idle_animation"), IdleAnimPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'idle_animation'"));
	}
	if (!Params->TryGetStringField(TEXT("walk_animation"), WalkAnimPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'walk_animation'"));
	}
	Params->TryGetStringField(TEXT("run_animation"), RunAnimPath);

	FString JumpAnimPath;
	Params->TryGetStringField(TEXT("jump_animation"), JumpAnimPath);
	bool bHasJump = !JumpAnimPath.IsEmpty();

	double WalkThreshold = 5.0, RunThreshold = 300.0;
	double CrossfadeDuration = 0.2;
	Params->TryGetNumberField(TEXT("walk_speed_threshold"), WalkThreshold);
	Params->TryGetNumberField(TEXT("run_speed_threshold"), RunThreshold);
	Params->TryGetNumberField(TEXT("crossfade_duration"), CrossfadeDuration);

	bool bHasRun = !RunAnimPath.IsEmpty();

	// === Part 1: Load assets ===
	UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, *AnimBPPath);
	if (!AnimBP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimBlueprint: %s"), *AnimBPPath));
	}

	UAnimSequence* IdleAnim = LoadObject<UAnimSequence>(nullptr, *IdleAnimPath);
	UAnimSequence* WalkAnim = LoadObject<UAnimSequence>(nullptr, *WalkAnimPath);
	UAnimSequence* RunAnim = bHasRun ? LoadObject<UAnimSequence>(nullptr, *RunAnimPath) : nullptr;

	if (!IdleAnim)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load idle animation: %s"), *IdleAnimPath));
	}
	if (!WalkAnim)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load walk animation: %s"), *WalkAnimPath));
	}
	if (bHasRun && !RunAnim)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load run animation: %s"), *RunAnimPath));
	}

	UAnimSequence* JumpAnim = bHasJump ? LoadObject<UAnimSequence>(nullptr, *JumpAnimPath) : nullptr;
	if (bHasJump && !JumpAnim)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load jump animation: %s"), *JumpAnimPath));
	}

	// === Part 2: Find AnimGraph ===
	UEdGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AnimBP->FunctionGraphs)
	{
		if (Graph->GetFName() == FName("AnimGraph"))
		{
			AnimGraph = Graph;
			break;
		}
	}
	if (!AnimGraph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph not found in AnimBlueprint"));
	}

	// Find Root (output) node
	UAnimGraphNode_Root* RootNode = nullptr;
	for (UEdGraphNode* Node : AnimGraph->Nodes)
	{
		if (UAnimGraphNode_Root* Root = Cast<UAnimGraphNode_Root>(Node))
		{
			RootNode = Root;
			break;
		}
	}
	if (!RootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph Root node not found"));
	}

	// === Part 2.5: Clean up existing AnimGraph nodes (except Root) ===
	// This allows setup_locomotion_state_machine to be called idempotently
	{
		TArray<UEdGraphNode*> NodesToRemove;
		for (UEdGraphNode* Node : AnimGraph->Nodes)
		{
			if (Node != RootNode)
			{
				NodesToRemove.Add(Node);
			}
		}
		for (UEdGraphNode* Node : NodesToRemove)
		{
			Node->BreakAllNodeLinks();
			AnimGraph->RemoveNode(Node);
		}
		// Also break any stale connections on Root input
		for (UEdGraphPin* Pin : RootNode->Pins)
		{
			Pin->BreakAllPinLinks();
		}
	}

	// Clean up existing EventGraph nodes (except the UpdateAnimation event which we reuse)
	if (AnimBP->UbergraphPages.Num() > 0)
	{
		UEdGraph* EG = AnimBP->UbergraphPages[0];
		TArray<UEdGraphNode*> EGNodesToRemove;
		for (UEdGraphNode* Node : EG->Nodes)
		{
			UK2Node_Event* EvNode = Cast<UK2Node_Event>(Node);
			if (EvNode && EvNode->EventReference.GetMemberName() == FName("BlueprintUpdateAnimation"))
			{
				// Keep the event node but break its connections so we rewire fresh
				EvNode->BreakAllNodeLinks();
				continue;
			}
			EGNodesToRemove.Add(Node);
		}
		for (UEdGraphNode* Node : EGNodesToRemove)
		{
			Node->BreakAllNodeLinks();
			EG->RemoveNode(Node);
		}
	}

	// === Part 3: Create State Machine ===
	UAnimGraphNode_StateMachine* SMNode = NewObject<UAnimGraphNode_StateMachine>(AnimGraph);
	SMNode->NodePosX = RootNode->NodePosX - 400;
	SMNode->NodePosY = RootNode->NodePosY;
	AnimGraph->AddNode(SMNode, true, false);
	SMNode->CreateNewGuid();
	SMNode->PostPlacedNewNode(); // Creates EditorStateMachineGraph
	SMNode->AllocateDefaultPins();

	// Connect SM output → Slot (DefaultSlot) → Root input
	// The Slot node enables montage/PlaySlotAnimationAsDynamicMontage playback
	UEdGraphPin* SMOutputPin = nullptr;
	for (UEdGraphPin* Pin : SMNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			SMOutputPin = Pin;
			break;
		}
	}
	UEdGraphPin* RootInputPin = nullptr;
	for (UEdGraphPin* Pin : RootNode->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			RootInputPin = Pin;
			break;
		}
	}

	// Create Slot node between SM and Root for montage playback
	UAnimGraphNode_Slot* SlotNode = NewObject<UAnimGraphNode_Slot>(AnimGraph);
	SlotNode->Node.SlotName = FName("DefaultSlot");
	SlotNode->Node.bAlwaysUpdateSourcePose = true; // CRITICAL: Keep source evaluating while montages play
	SlotNode->NodePosX = RootNode->NodePosX - 200;
	SlotNode->NodePosY = RootNode->NodePosY;
	AnimGraph->AddNode(SlotNode, true, false);
	SlotNode->CreateNewGuid();
	SlotNode->AllocateDefaultPins();

	UEdGraphPin* SlotInputPin = nullptr;
	UEdGraphPin* SlotOutputPin = nullptr;
	for (UEdGraphPin* Pin : SlotNode->Pins)
	{
		if (Pin->Direction == EGPD_Input) SlotInputPin = Pin;
		if (Pin->Direction == EGPD_Output) SlotOutputPin = Pin;
	}

	// Wire: SM → Slot → Root
	if (SMOutputPin && SlotInputPin)
	{
		SMOutputPin->MakeLinkTo(SlotInputPin);
	}
	if (SlotOutputPin && RootInputPin)
	{
		SlotOutputPin->MakeLinkTo(RootInputPin);
	}

	// === Part 4: Create States ===
	UAnimationStateMachineGraph* SMGraph = SMNode->EditorStateMachineGraph;
	if (!SMGraph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("State machine graph was not created"));
	}

	// Helper: create a state with an animation sequence player
	auto CreateState = [&](const FString& StateName, UAnimSequence* Anim, int32 PosX, int32 PosY) -> UAnimStateNode*
	{
		UAnimStateNode* StateNode = NewObject<UAnimStateNode>(SMGraph);
		StateNode->NodePosX = PosX;
		StateNode->NodePosY = PosY;
		SMGraph->AddNode(StateNode, true, false);
		StateNode->CreateNewGuid();
		StateNode->PostPlacedNewNode(); // Creates BoundGraph with StateResult
		StateNode->AllocateDefaultPins();

		// Rename the state's bound graph to our desired name
		if (StateNode->BoundGraph)
		{
			StateNode->BoundGraph->Rename(*StateName, nullptr);
		}

		// Add SequencePlayer inside the state's graph
		UAnimationStateGraph* StateGraph = Cast<UAnimationStateGraph>(StateNode->BoundGraph);
		if (StateGraph && Anim)
		{
			UAnimGraphNode_SequencePlayer* SeqPlayer = NewObject<UAnimGraphNode_SequencePlayer>(StateGraph);
			SeqPlayer->SetAnimationAsset(Anim);
			SeqPlayer->NodePosX = -200;
			SeqPlayer->NodePosY = 0;
			StateGraph->AddNode(SeqPlayer, true, false);
			SeqPlayer->CreateNewGuid();
			SeqPlayer->AllocateDefaultPins();

			// Connect SequencePlayer pose output → StateResult pose input
			if (StateGraph->MyResultNode)
			{
				UEdGraphPin* SeqOut = nullptr;
				for (UEdGraphPin* Pin : SeqPlayer->Pins)
				{
					if (Pin->Direction == EGPD_Output)
					{
						SeqOut = Pin;
						break;
					}
				}
				UEdGraphPin* ResultIn = nullptr;
				for (UEdGraphPin* Pin : StateGraph->MyResultNode->Pins)
				{
					if (Pin->Direction == EGPD_Input)
					{
						ResultIn = Pin;
						break;
					}
				}
				if (SeqOut && ResultIn)
				{
					SeqOut->MakeLinkTo(ResultIn);
				}
			}
		}

		return StateNode;
	};

	UAnimStateNode* IdleState = CreateState(TEXT("Idle"), IdleAnim, 200, 0);
	UAnimStateNode* WalkState = CreateState(TEXT("Walk"), WalkAnim, 500, -150);
	UAnimStateNode* RunState = bHasRun ? CreateState(TEXT("Run"), RunAnim, 800, 0) : nullptr;
	UAnimStateNode* JumpState = bHasJump ? CreateState(TEXT("Jump"), JumpAnim, 500, 200) : nullptr;

	// === Part 5: Connect Entry to Idle ===
	if (SMGraph->EntryNode && IdleState)
	{
		UEdGraphPin* EntryOutput = nullptr;
		for (UEdGraphPin* Pin : SMGraph->EntryNode->Pins)
		{
			if (Pin->Direction == EGPD_Output)
			{
				EntryOutput = Pin;
				break;
			}
		}
		UEdGraphPin* IdleInput = IdleState->GetInputPin();
		if (EntryOutput && IdleInput)
		{
			EntryOutput->MakeLinkTo(IdleInput);
		}
	}

	// === Part 6: Create Transitions ===
	auto CreateTransition = [&](UAnimStateNode* Source, UAnimStateNode* Target) -> UAnimStateTransitionNode*
	{
		UAnimStateTransitionNode* TransNode = NewObject<UAnimStateTransitionNode>(SMGraph);
		SMGraph->AddNode(TransNode, true, false);
		TransNode->CreateNewGuid();
		TransNode->PostPlacedNewNode(); // Creates BoundGraph for transition rule
		TransNode->AllocateDefaultPins(); // Must happen BEFORE CreateConnections (needs Pins array)
		TransNode->CreateConnections(Source, Target);
		TransNode->CrossfadeDuration = CrossfadeDuration;
		return TransNode;
	};

	UAnimStateTransitionNode* IdleToWalk = CreateTransition(IdleState, WalkState);
	UAnimStateTransitionNode* WalkToIdle = CreateTransition(WalkState, IdleState);
	UAnimStateTransitionNode* WalkToRun = bHasRun ? CreateTransition(WalkState, RunState) : nullptr;
	UAnimStateTransitionNode* RunToWalk = bHasRun ? CreateTransition(RunState, WalkState) : nullptr;

	// Jump transitions: any locomotion state -> Jump (when IsFalling), Jump -> Idle (when not falling)
	UAnimStateTransitionNode* IdleToJump = (bHasJump && JumpState) ? CreateTransition(IdleState, JumpState) : nullptr;
	UAnimStateTransitionNode* WalkToJump = (bHasJump && JumpState) ? CreateTransition(WalkState, JumpState) : nullptr;
	UAnimStateTransitionNode* RunToJump = (bHasJump && bHasRun && JumpState && RunState) ? CreateTransition(RunState, JumpState) : nullptr;
	UAnimStateTransitionNode* JumpToIdle = (bHasJump && JumpState) ? CreateTransition(JumpState, IdleState) : nullptr;

	// Jump transitions: fast entry (0.1s), slow exit (0.5s) for smooth landing
	if (IdleToJump) IdleToJump->CrossfadeDuration = 0.1f;
	if (WalkToJump) WalkToJump->CrossfadeDuration = 0.1f;
	if (RunToJump) RunToJump->CrossfadeDuration = 0.1f;
	if (JumpToIdle) JumpToIdle->CrossfadeDuration = 0.5f;

	// === Part 7: Add Speed variable to AnimBP ===
	int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(AnimBP, FName("Speed"));
	if (VarIndex == INDEX_NONE)
	{
		FBPVariableDescription SpeedVar;
		SpeedVar.VarName = FName("Speed");
		SpeedVar.VarGuid = FGuid::NewGuid();
		SpeedVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Real;
		SpeedVar.VarType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
		SpeedVar.DefaultValue = TEXT("0.0");
		SpeedVar.PropertyFlags |= CPF_BlueprintVisible;
		AnimBP->NewVariables.Add(SpeedVar);
	}

	if (bHasJump)
	{
		int32 FallingVarIndex = FBlueprintEditorUtils::FindNewVariableIndex(AnimBP, FName("IsFalling"));
		if (FallingVarIndex == INDEX_NONE)
		{
			FBPVariableDescription IsFallingVar;
			IsFallingVar.VarName = FName("IsFalling");
			IsFallingVar.VarGuid = FGuid::NewGuid();
			IsFallingVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			IsFallingVar.DefaultValue = TEXT("false");
			IsFallingVar.PropertyFlags |= CPF_BlueprintVisible;
			AnimBP->NewVariables.Add(IsFallingVar);
		}
	}

	// Part 7.5: Compile so Speed (and IsFalling) variables are baked into generated class
	// This is required before K2Node_VariableGet/Set can allocate pins for Speed/IsFalling
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);

	// === Part 8: Setup EventBlueprintUpdateAnimation for speed calculation ===
	UEdGraph* EventGraph = nullptr;
	if (AnimBP->UbergraphPages.Num() > 0)
	{
		EventGraph = AnimBP->UbergraphPages[0];
	}

	if (EventGraph)
	{
		// Find or create BlueprintUpdateAnimation event
		UK2Node_Event* UpdateAnimEvent = nullptr;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			UK2Node_Event* EvNode = Cast<UK2Node_Event>(Node);
			if (EvNode && EvNode->EventReference.GetMemberName() == FName("BlueprintUpdateAnimation"))
			{
				UpdateAnimEvent = EvNode;
				break;
			}
		}

		if (!UpdateAnimEvent)
		{
			UpdateAnimEvent = NewObject<UK2Node_Event>(EventGraph);
			UpdateAnimEvent->EventReference.SetExternalMember(
				FName("BlueprintUpdateAnimation"), UAnimInstance::StaticClass());
			UpdateAnimEvent->NodePosX = 0;
			UpdateAnimEvent->NodePosY = 400;
			EventGraph->AddNode(UpdateAnimEvent, true, false);
			UpdateAnimEvent->CreateNewGuid();
			UpdateAnimEvent->AllocateDefaultPins();
		}

		// TryGetPawnOwner (UAnimInstance member, called on self)
		UFunction* TryGetPawnOwnerFunc = UAnimInstance::StaticClass()->FindFunctionByName(FName("TryGetPawnOwner"));
		UK2Node_CallFunction* GetPawnNode = nullptr;
		if (TryGetPawnOwnerFunc)
		{
			GetPawnNode = NewObject<UK2Node_CallFunction>(EventGraph);
			GetPawnNode->SetFromFunction(TryGetPawnOwnerFunc);
			GetPawnNode->NodePosX = 300;
			GetPawnNode->NodePosY = 400;
			EventGraph->AddNode(GetPawnNode, true, false);
			GetPawnNode->CreateNewGuid();
			GetPawnNode->AllocateDefaultPins();
		}

		// GetVelocity (AActor member, called on pawn)
		UFunction* GetVelocityFunc = AActor::StaticClass()->FindFunctionByName(FName("GetVelocity"));
		UK2Node_CallFunction* GetVelNode = nullptr;
		if (GetVelocityFunc)
		{
			GetVelNode = NewObject<UK2Node_CallFunction>(EventGraph);
			GetVelNode->SetFromFunction(GetVelocityFunc);
			GetVelNode->NodePosX = 600;
			GetVelNode->NodePosY = 400;
			EventGraph->AddNode(GetVelNode, true, false);
			GetVelNode->CreateNewGuid();
			GetVelNode->AllocateDefaultPins();
		}

		// VSize (KismetMathLibrary, pure - no exec pins)
		UFunction* VSizeFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("VSize"));
		UK2Node_CallFunction* VSizeNode = nullptr;
		if (VSizeFunc)
		{
			VSizeNode = NewObject<UK2Node_CallFunction>(EventGraph);
			VSizeNode->SetFromFunction(VSizeFunc);
			VSizeNode->NodePosX = 900;
			VSizeNode->NodePosY = 400;
			EventGraph->AddNode(VSizeNode, true, false);
			VSizeNode->CreateNewGuid();
			VSizeNode->AllocateDefaultPins();
		}

		// Set Speed variable
		UK2Node_VariableSet* SetSpeedNode = NewObject<UK2Node_VariableSet>(EventGraph);
		SetSpeedNode->VariableReference.SetSelfMember(FName("Speed"));
		SetSpeedNode->NodePosX = 1200;
		SetSpeedNode->NodePosY = 400;
		EventGraph->AddNode(SetSpeedNode, true, false);
		SetSpeedNode->CreateNewGuid();
		SetSpeedNode->AllocateDefaultPins();

		// Wire execution: Event → SetSpeed (pure functions have no exec pins)
		UEdGraphPin* EventThen = UpdateAnimEvent->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* SetSpeedExec = SetSpeedNode->FindPin(UEdGraphSchema_K2::PN_Execute);
		if (EventThen && SetSpeedExec) EventThen->MakeLinkTo(SetSpeedExec);

		// Wire data chain (all pure nodes, evaluated lazily by Blueprint VM):
		// TryGetPawnOwner.ReturnValue → GetVelocity.self
		if (GetPawnNode && GetVelNode)
		{
			UEdGraphPin* PawnReturn = GetPawnNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
			UEdGraphPin* VelSelf = GetVelNode->FindPin(UEdGraphSchema_K2::PN_Self);
			if (PawnReturn && VelSelf) PawnReturn->MakeLinkTo(VelSelf);
		}

		// GetVelocity.ReturnValue → VSize.A
		if (GetVelNode && VSizeNode)
		{
			UEdGraphPin* VelReturn = GetVelNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
			UEdGraphPin* VSizeA = VSizeNode->FindPin(TEXT("A"));
			if (VelReturn && VSizeA) VelReturn->MakeLinkTo(VSizeA);
		}

		// VSize.ReturnValue → SetSpeed.Speed
		if (VSizeNode)
		{
			UEdGraphPin* VSizeReturn = VSizeNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
			UEdGraphPin* SpeedInput = SetSpeedNode->FindPin(FName("Speed"));
			if (!SpeedInput)
			{
				// Fallback: search all input pins for one matching Speed variable
				for (UEdGraphPin* Pin : SetSpeedNode->Pins)
				{
					if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
					{
						SpeedInput = Pin;
						break;
					}
				}
			}
			if (VSizeReturn && SpeedInput)
			{
				VSizeReturn->MakeLinkTo(SpeedInput);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: Failed to connect VSize→Speed (VSizeReturn=%d, SpeedInput=%d)"),
					VSizeReturn != nullptr, SpeedInput != nullptr);
			}
		}

		// === IsFalling chain (only when jump is enabled) ===
		// Uses pure functions only: TryGetPawnOwner → GetMovementComponent → IsFalling → SetIsFalling
		// GetMovementComponent is a UFUNCTION on APawn (returns UPawnMovementComponent*)
		// IsFalling is a UFUNCTION on UNavMovementComponent (parent of UPawnMovementComponent)
		// Both are const/pure = no exec pins needed, simple data chain
		if (bHasJump)
		{
			// GetMovementComponent (APawn UFUNCTION, pure - no exec pins)
			UFunction* GetMCFunc = APawn::StaticClass()->FindFunctionByName(FName("GetMovementComponent"));
			UK2Node_CallFunction* GetMCNode = nullptr;
			if (GetMCFunc)
			{
				GetMCNode = NewObject<UK2Node_CallFunction>(EventGraph);
				GetMCNode->SetFromFunction(GetMCFunc);
				GetMCNode->NodePosX = 300;
				GetMCNode->NodePosY = 600;
				EventGraph->AddNode(GetMCNode, true, false);
				GetMCNode->CreateNewGuid();
				GetMCNode->AllocateDefaultPins();

				// Wire TryGetPawnOwner.ReturnValue → GetMovementComponent.self
				if (GetPawnNode)
				{
					UEdGraphPin* PawnReturn = GetPawnNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
					UEdGraphPin* MCSelf = GetMCNode->FindPin(UEdGraphSchema_K2::PN_Self);
					if (PawnReturn && MCSelf) PawnReturn->MakeLinkTo(MCSelf);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: GetMovementComponent not found on APawn!"));
			}

			// IsFalling (UNavMovementComponent UFUNCTION, pure - no exec pins)
			UFunction* IsFallingFunc = UNavMovementComponent::StaticClass()->FindFunctionByName(FName("IsFalling"));
			UK2Node_CallFunction* IsFallingNode = nullptr;
			if (IsFallingFunc)
			{
				IsFallingNode = NewObject<UK2Node_CallFunction>(EventGraph);
				IsFallingNode->SetFromFunction(IsFallingFunc);
				IsFallingNode->NodePosX = 600;
				IsFallingNode->NodePosY = 600;
				EventGraph->AddNode(IsFallingNode, true, false);
				IsFallingNode->CreateNewGuid();
				IsFallingNode->AllocateDefaultPins();

				// Wire GetMovementComponent.ReturnValue → IsFalling.self
				if (GetMCNode)
				{
					UEdGraphPin* MCReturn = GetMCNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
					UEdGraphPin* IsFallingSelf = IsFallingNode->FindPin(UEdGraphSchema_K2::PN_Self);
					if (MCReturn && IsFallingSelf) MCReturn->MakeLinkTo(IsFallingSelf);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: IsFalling not found on UNavMovementComponent!"));
			}

			// Set IsFalling variable
			UK2Node_VariableSet* SetIsFallingNode = NewObject<UK2Node_VariableSet>(EventGraph);
			SetIsFallingNode->VariableReference.SetSelfMember(FName("IsFalling"));
			SetIsFallingNode->NodePosX = 900;
			SetIsFallingNode->NodePosY = 600;
			EventGraph->AddNode(SetIsFallingNode, true, false);
			SetIsFallingNode->CreateNewGuid();
			SetIsFallingNode->AllocateDefaultPins();

			// Wire IsFalling.ReturnValue → SetIsFalling input
			if (IsFallingNode)
			{
				UEdGraphPin* IsFallingReturn = IsFallingNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				UEdGraphPin* SetFallingInput = SetIsFallingNode->FindPin(FName("IsFalling"));
				if (!SetFallingInput)
				{
					// Fallback: find boolean input pin
					for (UEdGraphPin* Pin : SetIsFallingNode->Pins)
					{
						if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
						{
							SetFallingInput = Pin;
							break;
						}
					}
				}
				if (IsFallingReturn && SetFallingInput) IsFallingReturn->MakeLinkTo(SetFallingInput);
			}

			// Wire exec: SetSpeed.Then → SetIsFalling.Execute
			// (GetMovementComponent and IsFalling are pure functions with no exec pins)
			{
				UEdGraphPin* SetSpeedThen = SetSpeedNode->FindPin(UEdGraphSchema_K2::PN_Then);
				UEdGraphPin* SetFallingExec = SetIsFallingNode->FindPin(UEdGraphSchema_K2::PN_Execute);
				if (SetSpeedThen && SetFallingExec) SetSpeedThen->MakeLinkTo(SetFallingExec);
			}
		}
	}

	// === Part 9: Setup Transition Rules (speed comparison) ===
	// Find comparison function once (try multiple UE5 naming conventions)
	UFunction* GreaterFunc = nullptr;
	UFunction* LessFunc = nullptr;
	{
		TArray<FName> GreaterNames = {FName("Greater_DoubleDouble"), FName("Greater_FloatFloat")};
		TArray<FName> LessNames = {FName("Less_DoubleDouble"), FName("Less_FloatFloat")};
		for (const FName& Name : GreaterNames)
		{
			GreaterFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(Name);
			if (GreaterFunc)
			{
				UE_LOG(LogTemp, Display, TEXT("setup_locomotion: Found comparison func: %s"), *Name.ToString());
				break;
			}
		}
		for (const FName& Name : LessNames)
		{
			LessFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(Name);
			if (LessFunc) break;
		}
		if (!GreaterFunc || !LessFunc)
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: Comparison functions not found! Greater=%d, Less=%d"), GreaterFunc != nullptr, LessFunc != nullptr);
		}
	}

	auto SetupTransitionRule = [&](UAnimStateTransitionNode* TransNode, bool bGreaterThan, double Threshold)
	{
		if (!TransNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: TransNode is null"));
			return;
		}
		if (!TransNode->BoundGraph)
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: TransNode->BoundGraph is null"));
			return;
		}

		UAnimationTransitionGraph* TransGraph = Cast<UAnimationTransitionGraph>(TransNode->BoundGraph);
		if (!TransGraph || !TransGraph->MyResultNode)
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: TransGraph or MyResultNode is null"));
			return;
		}

		UFunction* CompFunc = bGreaterThan ? GreaterFunc : LessFunc;
		if (!CompFunc)
		{
			UE_LOG(LogTemp, Error, TEXT("setup_locomotion: No comparison function available for transition rule"));
			return;
		}

		// VariableGet for Speed
		UK2Node_VariableGet* SpeedGet = NewObject<UK2Node_VariableGet>(TransGraph);
		SpeedGet->VariableReference.SetSelfMember(FName("Speed"));
		SpeedGet->NodePosX = -300;
		SpeedGet->NodePosY = 0;
		TransGraph->AddNode(SpeedGet, true, false);
		SpeedGet->CreateNewGuid();
		SpeedGet->AllocateDefaultPins();

		UK2Node_CallFunction* CompNode = NewObject<UK2Node_CallFunction>(TransGraph);
		CompNode->SetFromFunction(CompFunc);
		CompNode->NodePosX = -100;
		CompNode->NodePosY = 0;
		TransGraph->AddNode(CompNode, true, false);
		CompNode->CreateNewGuid();
		CompNode->AllocateDefaultPins();

		// Set threshold as default value on B pin
		UEdGraphPin* CompB = CompNode->FindPin(TEXT("B"));
		if (CompB)
		{
			CompB->DefaultValue = FString::SanitizeFloat(Threshold);
		}

		// Wire: SpeedGet output → Comp.A
		UEdGraphPin* SpeedOut = SpeedGet->GetValuePin();
		UEdGraphPin* CompA = CompNode->FindPin(TEXT("A"));
		if (SpeedOut && CompA)
		{
			SpeedOut->MakeLinkTo(CompA);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: Failed to connect Speed→Comp.A (SpeedOut=%d, CompA=%d)"), SpeedOut != nullptr, CompA != nullptr);
		}

		// Wire: Comp.ReturnValue → TransitionResult input
		UEdGraphPin* CompReturn = CompNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
		// Find the boolean input pin on the transition result node
		UEdGraphPin* ResultPin = nullptr;
		for (UEdGraphPin* Pin : TransGraph->MyResultNode->Pins)
		{
			if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
			{
				ResultPin = Pin;
				break;
			}
		}
		if (!ResultPin)
		{
			ResultPin = TransGraph->MyResultNode->FindPin(TEXT("bCanEnterTransition"));
		}
		if (CompReturn && ResultPin)
		{
			CompReturn->MakeLinkTo(ResultPin);
			UE_LOG(LogTemp, Display, TEXT("setup_locomotion: Transition rule wired (threshold=%.1f, greater=%d)"), Threshold, bGreaterThan);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_locomotion: Failed to wire transition result (CompReturn=%d, ResultPin=%d)"),
				CompReturn != nullptr, ResultPin != nullptr);
			// Log all pins on result node for debugging
			for (UEdGraphPin* Pin : TransGraph->MyResultNode->Pins)
			{
				UE_LOG(LogTemp, Warning, TEXT("  ResultNode pin: %s, category=%s, dir=%d"),
					*Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString(), (int)Pin->Direction);
			}
		}
	};

	SetupTransitionRule(IdleToWalk, true, WalkThreshold);   // Speed > WalkThreshold
	SetupTransitionRule(WalkToIdle, false, WalkThreshold);  // Speed < WalkThreshold
	if (WalkToRun) SetupTransitionRule(WalkToRun, true, RunThreshold);
	if (RunToWalk) SetupTransitionRule(RunToWalk, false, RunThreshold);

	// Jump transition rules (IsFalling-based)
	if (bHasJump)
	{
		// Find Not_PreBool function for inverting IsFalling
		UFunction* NotBoolFunc = UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("Not_PreBool"));

		auto SetupBoolTransitionRule = [&](UAnimStateTransitionNode* TransNode, bool bInvert)
		{
			if (!TransNode || !TransNode->BoundGraph) return;

			UAnimationTransitionGraph* TransGraph = Cast<UAnimationTransitionGraph>(TransNode->BoundGraph);
			if (!TransGraph || !TransGraph->MyResultNode) return;

			// VariableGet for IsFalling
			UK2Node_VariableGet* FallingGet = NewObject<UK2Node_VariableGet>(TransGraph);
			FallingGet->VariableReference.SetSelfMember(FName("IsFalling"));
			FallingGet->NodePosX = -300;
			FallingGet->NodePosY = 0;
			TransGraph->AddNode(FallingGet, true, false);
			FallingGet->CreateNewGuid();
			FallingGet->AllocateDefaultPins();

			// Find transition result bool input
			UEdGraphPin* ResultPin = nullptr;
			for (UEdGraphPin* Pin : TransGraph->MyResultNode->Pins)
			{
				if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
				{
					ResultPin = Pin;
					break;
				}
			}
			if (!ResultPin)
			{
				ResultPin = TransGraph->MyResultNode->FindPin(TEXT("bCanEnterTransition"));
			}

			if (bInvert && NotBoolFunc)
			{
				// IsFalling -> NOT -> Result (for exit-jump transitions)
				UK2Node_CallFunction* NotNode = NewObject<UK2Node_CallFunction>(TransGraph);
				NotNode->SetFromFunction(NotBoolFunc);
				NotNode->NodePosX = -100;
				NotNode->NodePosY = 0;
				TransGraph->AddNode(NotNode, true, false);
				NotNode->CreateNewGuid();
				NotNode->AllocateDefaultPins();

				UEdGraphPin* FallingOut = FallingGet->GetValuePin();
				UEdGraphPin* NotA = NotNode->FindPin(TEXT("A"));
				if (FallingOut && NotA) FallingOut->MakeLinkTo(NotA);

				UEdGraphPin* NotReturn = NotNode->FindPin(UEdGraphSchema_K2::PN_ReturnValue);
				if (NotReturn && ResultPin) NotReturn->MakeLinkTo(ResultPin);
			}
			else
			{
				// IsFalling directly -> Result (for enter-jump transitions)
				UEdGraphPin* FallingOut = FallingGet->GetValuePin();
				if (FallingOut && ResultPin) FallingOut->MakeLinkTo(ResultPin);
			}
		};

		// Enter jump: IsFalling = true
		SetupBoolTransitionRule(IdleToJump, false);
		SetupBoolTransitionRule(WalkToJump, false);
		if (RunToJump) SetupBoolTransitionRule(RunToJump, false);

		// Exit jump: IsFalling = false (inverted)
		SetupBoolTransitionRule(JumpToIdle, true);
	}

	// === Part 10: Compile ===
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);

	// Check compile status
	bool bCompileSucceeded = (AnimBP->Status != EBlueprintStatus::BS_Error);
	UE_LOG(LogTemp, Display, TEXT("setup_locomotion: Final compile status=%d (0=UpToDate, 1=Dirty, 2=Error, 3=BeingCreated)"), (int)AnimBP->Status);

	// Build result
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), AnimBP->Status != EBlueprintStatus::BS_Error);
	ResultObj->SetNumberField(TEXT("compile_status"), (int)AnimBP->Status);
	ResultObj->SetStringField(TEXT("anim_blueprint"), AnimBPPath);
	if (AnimBP->Status == EBlueprintStatus::BS_Error)
	{
		UE_LOG(LogTemp, Error, TEXT("setup_locomotion: AnimBP compile FAILED (status=BS_Error). Open ABP in editor for details."));
		ResultObj->SetStringField(TEXT("error"), TEXT("AnimBP compilation failed - open ABP in editor to see errors"));
	}
	int32 JumpTransitionCount = bHasJump ? (3 + (bHasRun ? 1 : 0)) : 0;
	ResultObj->SetNumberField(TEXT("state_count"), 2 + (bHasRun ? 1 : 0) + (bHasJump ? 1 : 0));
	ResultObj->SetNumberField(TEXT("transition_count"), 2 + (bHasRun ? 2 : 0) + JumpTransitionCount);
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Locomotion state machine created with speed-based transitions%s%s"),
		bHasRun ? TEXT(" + run state") : TEXT(""),
		bHasJump ? TEXT(" + jump state (IsFalling-based)") : TEXT("")));
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetupBlendspaceLocomotion(const TSharedPtr<FJsonObject>& Params)
{
	// === Part 0: Parse parameters ===
	FString AnimBPPath;
	if (!Params->TryGetStringField(TEXT("anim_blueprint_path"), AnimBPPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint_path'"));
	}

	FString IdleAnimPath, WalkAnimPath;
	if (!Params->TryGetStringField(TEXT("idle_animation"), IdleAnimPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'idle_animation'"));
	}
	if (!Params->TryGetStringField(TEXT("walk_animation"), WalkAnimPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'walk_animation'"));
	}

	double MaxWalkSpeed = 300.0;
	Params->TryGetNumberField(TEXT("max_walk_speed"), MaxWalkSpeed);

	FString BlendSpacePath;
	Params->TryGetStringField(TEXT("blendspace_path"), BlendSpacePath);

	// === Part 1: Load assets ===
	UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, *AnimBPPath);
	if (!AnimBP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimBlueprint: %s"), *AnimBPPath));
	}

	UAnimSequence* IdleAnim = LoadObject<UAnimSequence>(nullptr, *IdleAnimPath);
	UAnimSequence* WalkAnim = LoadObject<UAnimSequence>(nullptr, *WalkAnimPath);
	if (!IdleAnim)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load idle animation: %s"), *IdleAnimPath));
	}
	if (!WalkAnim)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load walk animation: %s"), *WalkAnimPath));
	}

	USkeleton* Skeleton = AnimBP->TargetSkeleton.Get();
	if (!Skeleton)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimBlueprint has no target skeleton"));
	}

	// === Part 2: Reparent AnimBP to UEnemyAnimInstance ===
	// Load the C++ AnimInstance class from GameplayHelpers module at runtime (no compile-time dependency)
	UClass* EnemyAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Script/GameplayHelpers.EnemyAnimInstance"));
	bool bReparented = false;
	if (EnemyAnimClass)
	{
		if (AnimBP->ParentClass != EnemyAnimClass)
		{
			AnimBP->ParentClass = EnemyAnimClass;
			bReparented = true;
			UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Reparented AnimBP to UEnemyAnimInstance"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("setup_blendspace: UEnemyAnimInstance not found — AnimBP parent unchanged. Speed must be set via EventGraph."));
	}

	// Compile to regenerate generated class (needed for VariableGet to resolve Speed property)
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);

	// === Part 3: Create BlendSpace1D asset ===
	// Derive save path from AnimBP location if not provided
	if (BlendSpacePath.IsEmpty())
	{
		FString ABPDir = FPaths::GetPath(AnimBPPath);
		FString ABPName = FPaths::GetBaseFilename(AnimBPPath);
		FString BSName = ABPName;
		if (BSName.StartsWith(TEXT("ABP_")))
		{
			BSName = BSName.RightChop(4);
		}
		BlendSpacePath = ABPDir / (TEXT("BS_") + BSName + TEXT("_Locomotion"));
	}

	FString BSAssetName = FPaths::GetBaseFilename(BlendSpacePath);

	// Check if BlendSpace already exists — reuse it
	UBlendSpace1D* BS = LoadObject<UBlendSpace1D>(nullptr, *BlendSpacePath);
	if (BS)
	{
		// Clear existing samples so we rebuild from scratch
		while (BS->GetNumberOfBlendSamples() > 0)
		{
			BS->DeleteSample(0);
		}
		UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Reusing existing BlendSpace1D: %s"), *BlendSpacePath);
	}
	else
	{
		UPackage* BSPackage = CreatePackage(*BlendSpacePath);
		BS = NewObject<UBlendSpace1D>(BSPackage, *BSAssetName, RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(BS);
		UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Created new BlendSpace1D: %s"), *BlendSpacePath);
	}

	BS->SetSkeleton(Skeleton);

	// Log diagnostics for debugging
	UE_LOG(LogTemp, Display, TEXT("setup_blendspace: BS skeleton=%s, IdleAnim skeleton=%s, WalkAnim skeleton=%s"),
		BS->GetSkeleton() ? *BS->GetSkeleton()->GetPathName() : TEXT("NULL"),
		IdleAnim->GetSkeleton() ? *IdleAnim->GetSkeleton()->GetPathName() : TEXT("NULL"),
		WalkAnim->GetSkeleton() ? *WalkAnim->GetSkeleton()->GetPathName() : TEXT("NULL"));

	// Pre-configure BlendParameters axis range BEFORE AddSample.
	// Default range is [0,100] — if MaxWalkSpeed > 100, AddSample's IsSampleWithinBounds fails
	// because ExpandRangeForSample only extends in grid-delta increments.
	// BlendParameters is protected, so we use FProperty reflection.
	{
		FStructProperty* ParamProp = CastField<FStructProperty>(UBlendSpace::StaticClass()->FindPropertyByName(FName("BlendParameters")));
		if (ParamProp)
		{
			FBlendParameter* BlendParams = ParamProp->ContainerPtrToValuePtr<FBlendParameter>(BS);
			BlendParams[0].DisplayName = TEXT("Speed");
			BlendParams[0].Min = 0.0f;
			BlendParams[0].Max = (float)MaxWalkSpeed;
			BlendParams[0].GridNum = 4;
			UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Pre-configured BlendParameters[0] range 0-%.0f"), MaxWalkSpeed);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_blendspace: Failed to find BlendParameters property via reflection"));
		}
	}

	// Add samples (axis range already covers [0, MaxWalkSpeed])
	int32 IdleIdx = BS->AddSample(IdleAnim, FVector(0, 0, 0));
	int32 WalkIdx = BS->AddSample(WalkAnim, FVector(MaxWalkSpeed, 0, 0));

	UE_LOG(LogTemp, Display, TEXT("setup_blendspace: AddSample results: idle=%d, walk=%d"), IdleIdx, WalkIdx);

	// Fallback: if AddSample still fails, populate SampleData directly via reflection
	if (IdleIdx == INDEX_NONE || WalkIdx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("setup_blendspace: AddSample failed, using direct SampleData population fallback"));

		FArrayProperty* SampleDataProp = CastField<FArrayProperty>(UBlendSpace::StaticClass()->FindPropertyByName(FName("SampleData")));
		if (SampleDataProp)
		{
			FScriptArrayHelper ArrayHelper(SampleDataProp, SampleDataProp->ContainerPtrToValuePtr<void>(BS));
			ArrayHelper.EmptyValues();

			// Add idle sample
			int32 IdxIdle = ArrayHelper.AddValue();
			FBlendSample* IdleSamplePtr = (FBlendSample*)ArrayHelper.GetRawPtr(IdxIdle);
			IdleSamplePtr->Animation = IdleAnim;
			IdleSamplePtr->SampleValue = FVector(0, 0, 0);
			IdleSamplePtr->RateScale = 1.0f;
			IdleSamplePtr->bIsValid = true;

			// Add walk sample
			int32 IdxWalk = ArrayHelper.AddValue();
			FBlendSample* WalkSamplePtr = (FBlendSample*)ArrayHelper.GetRawPtr(IdxWalk);
			WalkSamplePtr->Animation = WalkAnim;
			WalkSamplePtr->SampleValue = FVector(MaxWalkSpeed, 0, 0);
			WalkSamplePtr->RateScale = 1.0f;
			WalkSamplePtr->bIsValid = true;

			IdleIdx = IdxIdle;
			WalkIdx = IdxWalk;
			UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Direct SampleData population succeeded (idle=%d, walk=%d)"), IdleIdx, WalkIdx);
		}

		if (IdleIdx == INDEX_NONE || WalkIdx == INDEX_NONE)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				TEXT("Failed to add samples to BlendSpace even with reflection fallback."));
		}
	}

	BS->ValidateSampleData();
	BS->GetPackage()->MarkPackageDirty();

	// Diagnostic: verify BS samples
	{
		int32 NumSamples = BS->GetNumberOfBlendSamples();
		UE_LOG(LogTemp, Display, TEXT("setup_blendspace: BS has %d samples after creation"), NumSamples);
		for (int32 i = 0; i < NumSamples; i++)
		{
			const FBlendSample& S = BS->GetBlendSample(i);
			UE_LOG(LogTemp, Display, TEXT("  Sample[%d]: anim=%s, value=(%.1f,%.1f,%.1f), valid=%d"),
				i, S.Animation ? *S.Animation->GetName() : TEXT("NULL"),
				S.SampleValue.X, S.SampleValue.Y, S.SampleValue.Z, S.bIsValid);
		}
		const FBlendParameter& BP0 = BS->GetBlendParameter(0);
		UE_LOG(LogTemp, Display, TEXT("  BlendParam[0]: name=%s, min=%.1f, max=%.1f, grid=%d"),
			*BP0.DisplayName, BP0.Min, BP0.Max, BP0.GridNum);
	}

	// Save BlendSpace asset
	FString BSFilename = FPackageName::LongPackageNameToFilename(BlendSpacePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs BSSaveArgs;
	BSSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(BS->GetPackage(), BS, *BSFilename, BSSaveArgs);

	// === Part 4: Find AnimGraph and clean up ===
	UEdGraph* AnimGraph = nullptr;
	for (UEdGraph* Graph : AnimBP->FunctionGraphs)
	{
		if (Graph->GetFName() == FName("AnimGraph"))
		{
			AnimGraph = Graph;
			break;
		}
	}
	if (!AnimGraph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph not found in AnimBlueprint"));
	}

	UAnimGraphNode_Root* RootNode = nullptr;
	for (UEdGraphNode* Node : AnimGraph->Nodes)
	{
		if (UAnimGraphNode_Root* Root = Cast<UAnimGraphNode_Root>(Node))
		{
			RootNode = Root;
			break;
		}
	}
	if (!RootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimGraph Root node not found"));
	}

	// Remove all AnimGraph nodes except Root (idempotent — safe to call repeatedly)
	{
		TArray<UEdGraphNode*> NodesToRemove;
		for (UEdGraphNode* Node : AnimGraph->Nodes)
		{
			if (Node != RootNode)
			{
				NodesToRemove.Add(Node);
			}
		}
		for (UEdGraphNode* Node : NodesToRemove)
		{
			Node->BreakAllNodeLinks();
			AnimGraph->RemoveNode(Node);
		}
		for (UEdGraphPin* Pin : RootNode->Pins)
		{
			Pin->BreakAllPinLinks();
		}
	}

	// === Part 4b: Rebuild EventGraph with Speed computation ===
	// BlueprintUpdateAnimation → TryGetPawnOwner → GetVelocity → VSizeXY → Set LocSpeed
	bool bEventGraphBuilt = false;
	if (AnimBP->UbergraphPages.Num() > 0)
	{
		UEdGraph* EG = AnimBP->UbergraphPages[0];

		// Clear existing EventGraph nodes
		{
			TArray<UEdGraphNode*> EGNodesToRemove;
			for (UEdGraphNode* Node : EG->Nodes)
			{
				EGNodesToRemove.Add(Node);
			}
			for (UEdGraphNode* Node : EGNodesToRemove)
			{
				Node->BreakAllNodeLinks();
				EG->RemoveNode(Node);
			}
		}

		// Ensure LocSpeed BP variable exists
		bool bHasLocSpeed = false;
		for (const FBPVariableDescription& Var : AnimBP->NewVariables)
		{
			if (Var.VarName == FName("LocSpeed"))
			{
				bHasLocSpeed = true;
				break;
			}
		}
		if (!bHasLocSpeed)
		{
			FBPVariableDescription SpeedVar;
			SpeedVar.VarName = FName("LocSpeed");
			SpeedVar.FriendlyName = TEXT("LocSpeed");
			SpeedVar.VarType.PinCategory = UEdGraphSchema_K2::PC_Real;
			SpeedVar.VarType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
			SpeedVar.PropertyFlags |= CPF_BlueprintVisible;
			AnimBP->NewVariables.Add(SpeedVar);
			UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Created LocSpeed BP variable"));
		}

		// Must recompile after adding variable so the generated class has the property
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
		FKismetEditorUtilities::CompileBlueprint(AnimBP);

		// 1. Event BlueprintUpdateAnimation
		UK2Node_Event* UpdateAnimEvent = NewObject<UK2Node_Event>(EG);
		UpdateAnimEvent->EventReference.SetExternalMember(FName("BlueprintUpdateAnimation"), UAnimInstance::StaticClass());
		UpdateAnimEvent->bOverrideFunction = true;
		UpdateAnimEvent->NodePosX = 0;
		UpdateAnimEvent->NodePosY = 0;
		EG->AddNode(UpdateAnimEvent, true, false);
		UpdateAnimEvent->CreateNewGuid();
		UpdateAnimEvent->AllocateDefaultPins();

		// 2. TryGetPawnOwner (pure — no exec pins)
		UK2Node_CallFunction* TryGetPawnNode = NewObject<UK2Node_CallFunction>(EG);
		{
			UFunction* Func = UAnimInstance::StaticClass()->FindFunctionByName(FName("TryGetPawnOwner"));
			if (Func)
			{
				TryGetPawnNode->SetFromFunction(Func);
			}
		}
		TryGetPawnNode->NodePosX = 200;
		TryGetPawnNode->NodePosY = 100;
		EG->AddNode(TryGetPawnNode, true, false);
		TryGetPawnNode->CreateNewGuid();
		TryGetPawnNode->AllocateDefaultPins();

		// 3. GetVelocity (pure — called on the Pawn return value)
		UK2Node_CallFunction* GetVelocityNode = NewObject<UK2Node_CallFunction>(EG);
		{
			UFunction* Func = AActor::StaticClass()->FindFunctionByName(FName("GetVelocity"));
			if (Func)
			{
				GetVelocityNode->SetFromFunction(Func);
			}
		}
		GetVelocityNode->NodePosX = 400;
		GetVelocityNode->NodePosY = 100;
		EG->AddNode(GetVelocityNode, true, false);
		GetVelocityNode->CreateNewGuid();
		GetVelocityNode->AllocateDefaultPins();

		// 4. VSizeXY (from KismetMathLibrary — pure, takes FVector, returns XY-plane magnitude)
		UK2Node_CallFunction* VSize2DNode = NewObject<UK2Node_CallFunction>(EG);
		{
			UFunction* Func = UKismetMathLibrary::StaticClass()->FindFunctionByName(FName("VSizeXY"));
			if (Func)
			{
				VSize2DNode->SetFromFunction(Func);
			}
		}
		VSize2DNode->NodePosX = 600;
		VSize2DNode->NodePosY = 100;
		EG->AddNode(VSize2DNode, true, false);
		VSize2DNode->CreateNewGuid();
		VSize2DNode->AllocateDefaultPins();

		// 5. Set LocSpeed variable
		UK2Node_VariableSet* SetLocSpeedNode = NewObject<UK2Node_VariableSet>(EG);
		SetLocSpeedNode->VariableReference.SetSelfMember(FName("LocSpeed"));
		SetLocSpeedNode->NodePosX = 800;
		SetLocSpeedNode->NodePosY = 0;
		EG->AddNode(SetLocSpeedNode, true, false);
		SetLocSpeedNode->CreateNewGuid();
		SetLocSpeedNode->AllocateDefaultPins();

		// === Wire exec chain ===
		// Event.Then → SetLocSpeed.Execute (pure functions have no exec pins)
		UEdGraphPin* EventThenPin = UpdateAnimEvent->FindPin(UEdGraphSchema_K2::PN_Then);
		UEdGraphPin* SetExecPin = SetLocSpeedNode->GetExecPin();
		if (EventThenPin && SetExecPin)
		{
			EventThenPin->MakeLinkTo(SetExecPin);
		}

		// === Wire data chain ===
		// TryGetPawnOwner.ReturnValue → GetVelocity.self
		UEdGraphPin* PawnOutPin = TryGetPawnNode->GetReturnValuePin();
		UEdGraphPin* VelSelfPin = GetVelocityNode->FindPin(UEdGraphSchema_K2::PN_Self);
		if (PawnOutPin && VelSelfPin)
		{
			PawnOutPin->MakeLinkTo(VelSelfPin);
		}

		// GetVelocity.ReturnValue → VSize2D.A
		UEdGraphPin* VelReturnPin = GetVelocityNode->GetReturnValuePin();
		UEdGraphPin* VSize2DInputPin = VSize2DNode->FindPin(TEXT("A"));
		if (VelReturnPin && VSize2DInputPin)
		{
			VelReturnPin->MakeLinkTo(VSize2DInputPin);
		}

		// VSize2D.ReturnValue → SetLocSpeed.LocSpeed
		UEdGraphPin* VSize2DReturnPin = VSize2DNode->GetReturnValuePin();
		UEdGraphPin* SetLocSpeedValuePin = SetLocSpeedNode->FindPin(FName("LocSpeed"));
		if (VSize2DReturnPin && SetLocSpeedValuePin)
		{
			VSize2DReturnPin->MakeLinkTo(SetLocSpeedValuePin);
		}

		bEventGraphBuilt = true;
		UE_LOG(LogTemp, Display, TEXT("setup_blendspace: EventGraph rebuilt with Speed computation (5 nodes, 4 data + 1 exec wire)"));

		// Log all wiring for diagnostic
		UE_LOG(LogTemp, Display, TEXT("  EventThen→SetExec: %d"), EventThenPin && SetExecPin && EventThenPin->LinkedTo.Contains(SetExecPin));
		UE_LOG(LogTemp, Display, TEXT("  Pawn→VelSelf: %d"), PawnOutPin && VelSelfPin && PawnOutPin->LinkedTo.Contains(VelSelfPin));
		UE_LOG(LogTemp, Display, TEXT("  VelReturn→VSizeXY.A: %d"), VelReturnPin && VSize2DInputPin && VelReturnPin->LinkedTo.Contains(VSize2DInputPin));
		UE_LOG(LogTemp, Display, TEXT("  VSizeXY.Return→SetLocSpeed: %d"), VSize2DReturnPin && SetLocSpeedValuePin && VSize2DReturnPin->LinkedTo.Contains(SetLocSpeedValuePin));
	}

	// === Part 5: Create AnimGraph: BlendSpacePlayer → Slot(DefaultSlot) → Root ===
	// BlendSpacePlayer node
	UAnimGraphNode_BlendSpacePlayer* BSNode = NewObject<UAnimGraphNode_BlendSpacePlayer>(AnimGraph);
	BSNode->SetAnimationAsset(BS);
	BSNode->NodePosX = RootNode->NodePosX - 600;
	BSNode->NodePosY = RootNode->NodePosY;
	AnimGraph->AddNode(BSNode, true, false);
	BSNode->CreateNewGuid();
	BSNode->AllocateDefaultPins();

	// Slot node (montage overlay for attacks, hit-react, death)
	UAnimGraphNode_Slot* SlotNode = NewObject<UAnimGraphNode_Slot>(AnimGraph);
	SlotNode->Node.SlotName = FName("DefaultSlot");
	SlotNode->Node.bAlwaysUpdateSourcePose = true; // CRITICAL: Keep BlendSpace evaluating while montages play
	SlotNode->NodePosX = RootNode->NodePosX - 200;
	SlotNode->NodePosY = RootNode->NodePosY;
	AnimGraph->AddNode(SlotNode, true, false);
	SlotNode->CreateNewGuid();
	SlotNode->AllocateDefaultPins();

	// Find pins
	UEdGraphPin* BSOutputPin = nullptr;
	for (UEdGraphPin* Pin : BSNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			BSOutputPin = Pin;
			break;
		}
	}

	UEdGraphPin* SlotInputPin = nullptr;
	UEdGraphPin* SlotOutputPin = nullptr;
	for (UEdGraphPin* Pin : SlotNode->Pins)
	{
		if (Pin->Direction == EGPD_Input) SlotInputPin = Pin;
		if (Pin->Direction == EGPD_Output) SlotOutputPin = Pin;
	}

	UEdGraphPin* RootInputPin = nullptr;
	for (UEdGraphPin* Pin : RootNode->Pins)
	{
		if (Pin->Direction == EGPD_Input)
		{
			RootInputPin = Pin;
			break;
		}
	}

	// Wire: BlendSpacePlayer → Slot → Root
	if (BSOutputPin && SlotInputPin) BSOutputPin->MakeLinkTo(SlotInputPin);
	if (SlotOutputPin && RootInputPin) SlotOutputPin->MakeLinkTo(RootInputPin);

	// === Part 6: Wire LocSpeed BP variable to BlendSpacePlayer X pin ===
	bool bSpeedWired = false;
	{
		UK2Node_VariableGet* LocSpeedGetNode = NewObject<UK2Node_VariableGet>(AnimGraph);
		LocSpeedGetNode->VariableReference.SetSelfMember(FName("LocSpeed"));
		LocSpeedGetNode->NodePosX = BSNode->NodePosX - 200;
		LocSpeedGetNode->NodePosY = BSNode->NodePosY + 100;
		AnimGraph->AddNode(LocSpeedGetNode, true, false);
		LocSpeedGetNode->CreateNewGuid();
		LocSpeedGetNode->AllocateDefaultPins();

		// Find X pin on BlendSpacePlayer
		UEdGraphPin* BSXPin = BSNode->FindPin(TEXT("X"));
		if (!BSXPin)
		{
			for (UEdGraphPin* Pin : BSNode->Pins)
			{
				if (Pin->Direction == EGPD_Input &&
					(Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real ||
					 Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Float))
				{
					BSXPin = Pin;
					UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Found X pin via fallback: %s"), *Pin->PinName.ToString());
					break;
				}
			}
		}

		UEdGraphPin* LocSpeedOut = LocSpeedGetNode->GetValuePin();
		if (LocSpeedOut && BSXPin)
		{
			LocSpeedOut->MakeLinkTo(BSXPin);
			bSpeedWired = true;
			UE_LOG(LogTemp, Display, TEXT("setup_blendspace: LocSpeed BP var → BS.X wired successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("setup_blendspace: Failed to wire LocSpeed→BS.X (LocSpeedOut=%d, BSXPin=%d)"),
				LocSpeedOut != nullptr, BSXPin != nullptr);
		}
	}

	// === Part 6.5: Remove stale BP variables (keep LocSpeed, remove others) ===
	{
		TArray<FName> VarsToRemove;
		for (const FBPVariableDescription& Var : AnimBP->NewVariables)
		{
			if (Var.VarName != FName("LocSpeed"))
			{
				VarsToRemove.Add(Var.VarName);
			}
		}
		for (const FName& VarName : VarsToRemove)
		{
			FBlueprintEditorUtils::RemoveMemberVariable(AnimBP, VarName);
			UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Removed stale BP variable: %s"), *VarName.ToString());
		}
	}

	// === Part 7: Compile and Save ===
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
	FKismetEditorUtilities::CompileBlueprint(AnimBP);

	bool bCompileSucceeded = (AnimBP->Status != EBlueprintStatus::BS_Error);
	UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Final compile status=%d (3=UpToDate, 2=Error)"), (int)AnimBP->Status);

	// Save AnimBP to disk (prevents loss on editor restart)
	if (bCompileSucceeded)
	{
		FString ABPFilename = FPackageName::LongPackageNameToFilename(AnimBPPath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs ABPSaveArgs;
		ABPSaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(AnimBP->GetPackage(), AnimBP, *ABPFilename, ABPSaveArgs);
		UE_LOG(LogTemp, Display, TEXT("setup_blendspace: Saved AnimBP to disk: %s"), *ABPFilename);
	}

	// Build result
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), bCompileSucceeded);
	ResultObj->SetNumberField(TEXT("compile_status"), (int)AnimBP->Status);
	ResultObj->SetStringField(TEXT("anim_blueprint"), AnimBPPath);
	ResultObj->SetStringField(TEXT("blendspace_path"), BlendSpacePath);
	ResultObj->SetBoolField(TEXT("reparented"), bReparented);
	ResultObj->SetBoolField(TEXT("speed_wired"), bSpeedWired);
	ResultObj->SetStringField(TEXT("parent_class"), EnemyAnimClass ? TEXT("UEnemyAnimInstance") : TEXT("UAnimInstance"));
	if (!bCompileSucceeded)
	{
		ResultObj->SetStringField(TEXT("error"), TEXT("AnimBP compilation failed — open ABP in editor to see errors"));
	}
	ResultObj->SetBoolField(TEXT("event_graph_built"), bEventGraphBuilt);
	ResultObj->SetStringField(TEXT("message"), FString::Printf(
		TEXT("BlendSpace1D locomotion: [BS(%s) LocSpeed:0=Idle,%.0f=Walk] → Slot(DefaultSlot) → Output. Speed computed in EventGraph (TryGetPawnOwner→GetVelocity→VSizeXY→LocSpeed)."),
		*BSAssetName, MaxWalkSpeed));
	return ResultObj;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetCharacterProperties(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
	}

	// Load the Blueprint
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!BP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load Blueprint: %s"), *BlueprintPath));
	}

	if (!BP->GeneratedClass)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no GeneratedClass - compile it first"));
	}

	ACharacter* CDO = Cast<ACharacter>(BP->GeneratedClass->GetDefaultObject());
	if (!CDO)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint is not a Character Blueprint"));
	}

	USkeletalMeshComponent* MeshComp = CDO->GetMesh();
	if (!MeshComp)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Character has no SkeletalMeshComponent"));
	}

	TArray<FString> ChangesApplied;

	// Set AnimBlueprint if provided (pass "None" to clear)
	FString AnimBPPath;
	if (Params->TryGetStringField(TEXT("anim_blueprint_path"), AnimBPPath) && !AnimBPPath.IsEmpty())
	{
		if (AnimBPPath.Equals(TEXT("None"), ESearchCase::IgnoreCase) || AnimBPPath.Equals(TEXT("null"), ESearchCase::IgnoreCase))
		{
			// Clear AnimBP — switch to no animation blueprint
			MeshComp->SetAnimInstanceClass(nullptr);
			MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			ChangesApplied.Add(TEXT("AnimBP cleared (set to None), mode set to AnimationSingleNode"));
		}
		else
		{
			UAnimBlueprint* AnimBP = LoadObject<UAnimBlueprint>(nullptr, *AnimBPPath);
			if (AnimBP && AnimBP->GeneratedClass)
			{
				MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
				MeshComp->SetAnimInstanceClass(AnimBP->GeneratedClass);
				ChangesApplied.Add(FString::Printf(TEXT("AnimBP set to %s (mode=AnimationBlueprint)"), *AnimBPPath));
			}
			else
			{
				return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimBlueprint: %s"), *AnimBPPath));
			}
		}
	}

	// Set SkeletalMesh if provided
	FString MeshPath;
	if (Params->TryGetStringField(TEXT("skeletal_mesh_path"), MeshPath) && !MeshPath.IsEmpty())
	{
		USkeletalMesh* SkelMesh = LoadObject<USkeletalMesh>(nullptr, *MeshPath);
		if (SkelMesh)
		{
			MeshComp->SetSkeletalMesh(SkelMesh);
			ChangesApplied.Add(FString::Printf(TEXT("SkeletalMesh set to %s"), *MeshPath));
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load SkeletalMesh: %s"), *MeshPath));
		}
	}

	// Set mesh relative transform if provided
	double MeshOffsetZ = 0;
	if (Params->TryGetNumberField(TEXT("mesh_offset_z"), MeshOffsetZ))
	{
		FVector Loc = MeshComp->GetRelativeLocation();
		Loc.Z = MeshOffsetZ;
		MeshComp->SetRelativeLocation(Loc);
		ChangesApplied.Add(FString::Printf(TEXT("Mesh Z offset set to %.1f"), MeshOffsetZ));
	}

	// Set capsule half height if provided
	double CapsuleHH = 0;
	if (Params->TryGetNumberField(TEXT("capsule_half_height"), CapsuleHH))
	{
		UCapsuleComponent* Capsule = CDO->GetCapsuleComponent();
		if (Capsule)
		{
			Capsule->SetCapsuleHalfHeight(CapsuleHH);
			ChangesApplied.Add(FString::Printf(TEXT("CapsuleHalfHeight set to %.1f"), CapsuleHH));
		}
	}

	double CapsuleR = 0;
	if (Params->TryGetNumberField(TEXT("capsule_radius"), CapsuleR))
	{
		UCapsuleComponent* Capsule = CDO->GetCapsuleComponent();
		if (Capsule)
		{
			Capsule->SetCapsuleRadius(CapsuleR);
			ChangesApplied.Add(FString::Printf(TEXT("CapsuleRadius set to %.1f"), CapsuleR));
		}
	}

	// Set mesh relative scale if provided (process BEFORE auto_fit so fit uses new scale)
	double MeshScale = 0;
	if (Params->TryGetNumberField(TEXT("mesh_scale"), MeshScale) && MeshScale > 0)
	{
		MeshComp->SetRelativeScale3D(FVector(MeshScale));
		ChangesApplied.Add(FString::Printf(TEXT("Mesh scale set to %.2f"), MeshScale));
	}

	// Auto-fit capsule to mesh bounds (accounts for mesh component scale)
	bool bAutoFit = false;
	if (Params->TryGetBoolField(TEXT("auto_fit_capsule"), bAutoFit) && bAutoFit)
	{
		USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset();
		if (SkelMesh)
		{
			// Use GetImportedBounds() for accurate mesh geometry bounds.
			// GetBounds() returns worst-case skeleton bounds (all bones extended)
			// which are drastically inflated and cause wrong capsule sizing.
			FBoxSphereBounds MeshBounds = SkelMesh->GetImportedBounds();

			// Fallback: if imported bounds are zero (shouldn't happen), try GetBounds
			if (MeshBounds.BoxExtent.Z < 1.0f)
			{
				MeshBounds = SkelMesh->GetBounds();
				ChangesApplied.Add(TEXT("auto_fit: ImportedBounds was zero, fell back to GetBounds"));
			}

			// Account for mesh component's relative scale (e.g., mesh_scale=3 for huge enemies)
			FVector MeshRelScale = MeshComp->GetRelativeScale3D();
			float ScaleZ = MeshRelScale.Z;
			float ScaleXY = FMath::Max(MeshRelScale.X, MeshRelScale.Y);

			// Full mesh dimensions (scaled)
			float MeshHeight = MeshBounds.BoxExtent.Z * 2.0f * ScaleZ;
			float MeshWidth = FMath::Max(MeshBounds.BoxExtent.X, MeshBounds.BoxExtent.Y) * 2.0f * ScaleXY;

			// The mesh's lowest and highest points in local space (scaled)
			float MeshMinZ = (MeshBounds.Origin.Z - MeshBounds.BoxExtent.Z) * ScaleZ;
			float MeshMaxZ = (MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z) * ScaleZ;

			// Capsule half-height = half the mesh height with 5% margin
			float FitHalfHeight = (MeshHeight / 2.0f) * 1.05f;

			// Radius from the widest axis, clamped to not exceed half-height
			float FitRadius = FMath::Min((MeshWidth / 2.0f) * 1.05f, FitHalfHeight);
			FitRadius = FMath::Max(FitRadius, 20.0f); // minimum sane radius

			UCapsuleComponent* Capsule = CDO->GetCapsuleComponent();
			if (Capsule)
			{
				Capsule->SetCapsuleHalfHeight(FitHalfHeight);
				Capsule->SetCapsuleRadius(FitRadius);
			}

			// Position mesh so its BOTTOM aligns with capsule BOTTOM.
			// Capsule bottom (relative to capsule center) = -HalfHeight
			// Mesh bottom (relative to mesh origin) = MeshMinZ
			// We need: mesh_offset_z + MeshMinZ = -FitHalfHeight
			// Therefore: mesh_offset_z = -FitHalfHeight - MeshMinZ
			float AutoFitMeshZ = -FitHalfHeight - MeshMinZ;

			FVector Loc = MeshComp->GetRelativeLocation();
			Loc.Z = AutoFitMeshZ;
			MeshComp->SetRelativeLocation(Loc);

			ChangesApplied.Add(FString::Printf(
				TEXT("Auto-fit capsule: HH=%.1f, R=%.1f, MeshZ=%.1f (ImportedBounds: Origin=(%.1f,%.1f,%.1f), Extent=(%.1f,%.1f,%.1f), MeshMinZ=%.1f, MeshMaxZ=%.1f, Height=%.1f, Width=%.1f)"),
				FitHalfHeight, FitRadius, AutoFitMeshZ,
				MeshBounds.Origin.X, MeshBounds.Origin.Y, MeshBounds.Origin.Z,
				MeshBounds.BoxExtent.X, MeshBounds.BoxExtent.Y, MeshBounds.BoxExtent.Z,
				MeshMinZ, MeshMaxZ, MeshHeight, MeshWidth));
		}
		else
		{
			ChangesApplied.Add(TEXT("auto_fit_capsule: No skeletal mesh assigned, skipped"));
		}
	}

	if (ChangesApplied.Num() == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No properties provided to change (use anim_blueprint_path, skeletal_mesh_path, mesh_offset_z, capsule_half_height, capsule_radius, mesh_scale, or auto_fit_capsule)"));
	}

	// Compile and mark dirty
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	BP->GetPackage()->MarkPackageDirty();

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("blueprint"), BlueprintPath);

	TArray<TSharedPtr<FJsonValue>> ChangesArray;
	for (const FString& Change : ChangesApplied)
	{
		ChangesArray.Add(MakeShared<FJsonValueString>(Change));
	}
	Result->SetArrayField(TEXT("changes"), ChangesArray);
	return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetAnimSequenceRootMotion(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimSequencePath;
    if (!Params->TryGetStringField(TEXT("anim_sequence_path"), AnimSequencePath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_sequence_path' parameter"));
    }

    bool bEnableRootMotion = false;
    if (!Params->TryGetBoolField(TEXT("enable_root_motion"), bEnableRootMotion))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'enable_root_motion' parameter"));
    }

    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AnimSequencePath);
    UAnimSequence* AnimSequence = Cast<UAnimSequence>(LoadedAsset);
    if (!AnimSequence)
    {
        AnimSequence = LoadObject<UAnimSequence>(nullptr, *AnimSequencePath);
    }
    if (!AnimSequence)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimSequence: %s"), *AnimSequencePath));
    }

    const bool bPrevious = AnimSequence->bEnableRootMotion;
    AnimSequence->Modify();
    AnimSequence->bEnableRootMotion = bEnableRootMotion;
    AnimSequence->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(AnimSequence);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("anim_sequence_path"), AnimSequencePath);
    Result->SetBoolField(TEXT("previous_enable_root_motion"), bPrevious);
    Result->SetBoolField(TEXT("enable_root_motion"), AnimSequence->bEnableRootMotion);
    Result->SetStringField(TEXT("message"), TEXT("AnimSequence root motion updated"));
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetAnimStateAlwaysResetOnEntry(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPPath;
    if (!Params->TryGetStringField(TEXT("anim_blueprint_path"), AnimBPPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint_path' parameter"));
    }

    FString StateName;
    if (!Params->TryGetStringField(TEXT("state_name"), StateName) || StateName.IsEmpty())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'state_name' parameter"));
    }

    FString StateMachineName;
    Params->TryGetStringField(TEXT("state_machine_name"), StateMachineName);

    bool bAlwaysResetOnEntry = false;
    if (!Params->TryGetBoolField(TEXT("always_reset_on_entry"), bAlwaysResetOnEntry))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'always_reset_on_entry' parameter"));
    }

    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AnimBPPath);
    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(LoadedAsset);
    if (!AnimBP)
    {
        AnimBP = LoadObject<UAnimBlueprint>(nullptr, *AnimBPPath);
    }
    if (!AnimBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimBlueprint: %s"), *AnimBPPath));
    }

    TArray<UAnimGraphNode_StateMachine*> MatchingStateMachines;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (!Graph || Graph->GetFName() != FName("AnimGraph"))
        {
            continue;
        }

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
            if (!SMNode || !SMNode->EditorStateMachineGraph)
            {
                continue;
            }

            if (StateMachineName.IsEmpty() ||
                SMNode->EditorStateMachineGraph->GetName().Equals(StateMachineName, ESearchCase::IgnoreCase))
            {
                MatchingStateMachines.Add(SMNode);
            }
        }
    }

    if (MatchingStateMachines.Num() == 0)
    {
        if (StateMachineName.IsEmpty())
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No state machines found in AnimBlueprint AnimGraph"));
        }
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("State machine not found: %s"), *StateMachineName));
    }

    UAnimStateNode* FoundState = nullptr;
    FString FoundStateMachineName;
    for (UAnimGraphNode_StateMachine* SMNode : MatchingStateMachines)
    {
        UAnimationStateMachineGraph* SMGraph = SMNode->EditorStateMachineGraph;
        if (!SMGraph)
        {
            continue;
        }

        for (UEdGraphNode* Node : SMGraph->Nodes)
        {
            UAnimStateNode* StateNode = Cast<UAnimStateNode>(Node);
            if (!StateNode)
            {
                continue;
            }

            const FString CandidateName = StateNode->BoundGraph ? StateNode->BoundGraph->GetName() : StateNode->GetName();
            if (CandidateName.Equals(StateName, ESearchCase::IgnoreCase))
            {
                FoundState = StateNode;
                FoundStateMachineName = SMGraph->GetName();
                break;
            }
        }

        if (FoundState)
        {
            break;
        }
    }

    if (!FoundState)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("State not found: %s"), *StateName));
    }

    const bool bPrevious = FoundState->bAlwaysResetOnEntry;
    FoundState->Modify();
    FoundState->bAlwaysResetOnEntry = bAlwaysResetOnEntry;

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
    FKismetEditorUtilities::CompileBlueprint(AnimBP);
    AnimBP->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(AnimBP);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), AnimBP->Status != EBlueprintStatus::BS_Error);
    Result->SetStringField(TEXT("anim_blueprint_path"), AnimBPPath);
    Result->SetStringField(TEXT("state_machine_name"), FoundStateMachineName);
    Result->SetStringField(TEXT("state_name"), StateName);
    Result->SetBoolField(TEXT("previous_always_reset_on_entry"), bPrevious);
    Result->SetBoolField(TEXT("always_reset_on_entry"), FoundState->bAlwaysResetOnEntry);
    Result->SetNumberField(TEXT("compile_status"), static_cast<int32>(AnimBP->Status));
    if (AnimBP->Status == EBlueprintStatus::BS_Error)
    {
        Result->SetStringField(TEXT("warning"), TEXT("AnimBlueprint compiled with errors; check editor compiler output"));
    }
    return Result;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintCommands::HandleSetStateMachineMaxTransitionsPerFrame(const TSharedPtr<FJsonObject>& Params)
{
    FString AnimBPPath;
    if (!Params->TryGetStringField(TEXT("anim_blueprint_path"), AnimBPPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'anim_blueprint_path' parameter"));
    }

    double MaxTransitionsPerFrameRaw = 1.0;
    if (!Params->TryGetNumberField(TEXT("max_transitions_per_frame"), MaxTransitionsPerFrameRaw))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'max_transitions_per_frame' parameter"));
    }
    int32 MaxTransitionsPerFrame = static_cast<int32>(MaxTransitionsPerFrameRaw);
    MaxTransitionsPerFrame = FMath::Clamp(MaxTransitionsPerFrame, 1, 128);

    FString StateMachineName;
    Params->TryGetStringField(TEXT("state_machine_name"), StateMachineName);

    UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AnimBPPath);
    UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(LoadedAsset);
    if (!AnimBP)
    {
        AnimBP = LoadObject<UAnimBlueprint>(nullptr, *AnimBPPath);
    }
    if (!AnimBP)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimBlueprint: %s"), *AnimBPPath));
    }

    UAnimGraphNode_StateMachine* TargetStateMachine = nullptr;
    for (UEdGraph* Graph : AnimBP->FunctionGraphs)
    {
        if (!Graph || Graph->GetFName() != FName("AnimGraph"))
        {
            continue;
        }

        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UAnimGraphNode_StateMachine* SMNode = Cast<UAnimGraphNode_StateMachine>(Node);
            if (!SMNode || !SMNode->EditorStateMachineGraph)
            {
                continue;
            }

            if (StateMachineName.IsEmpty() ||
                SMNode->EditorStateMachineGraph->GetName().Equals(StateMachineName, ESearchCase::IgnoreCase))
            {
                TargetStateMachine = SMNode;
                break;
            }
        }

        if (TargetStateMachine)
        {
            break;
        }
    }

    if (!TargetStateMachine)
    {
        if (StateMachineName.IsEmpty())
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No state machine found in AnimBlueprint AnimGraph"));
        }
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("State machine not found: %s"), *StateMachineName));
    }

    const int32 PreviousValue = TargetStateMachine->Node.MaxTransitionsPerFrame;
    TargetStateMachine->Modify();
    TargetStateMachine->Node.MaxTransitionsPerFrame = MaxTransitionsPerFrame;

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBP);
    FKismetEditorUtilities::CompileBlueprint(AnimBP);
    AnimBP->MarkPackageDirty();
    UEditorAssetLibrary::SaveLoadedAsset(AnimBP);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), AnimBP->Status != EBlueprintStatus::BS_Error);
    Result->SetStringField(TEXT("anim_blueprint_path"), AnimBPPath);
    Result->SetStringField(TEXT("state_machine_name"), TargetStateMachine->EditorStateMachineGraph->GetName());
    Result->SetNumberField(TEXT("previous_max_transitions_per_frame"), PreviousValue);
    Result->SetNumberField(TEXT("max_transitions_per_frame"), TargetStateMachine->Node.MaxTransitionsPerFrame);
    Result->SetNumberField(TEXT("compile_status"), static_cast<int32>(AnimBP->Status));
    if (AnimBP->Status == EBlueprintStatus::BS_Error)
    {
        Result->SetStringField(TEXT("warning"), TEXT("AnimBlueprint compiled with errors; check editor compiler output"));
    }
    return Result;
}
