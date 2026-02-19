#include "EpicUnrealMCPBridge.h"
#include "MCPServerRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Containers/Ticker.h"
// Add Blueprint related includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// UE5.5 correct includes
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
// Blueprint Graph specific includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "GameFramework/InputSettings.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
// Screenshot 2-phase capture includes
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LevelEditorViewport.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h"
// Include our new command handler classes
#include "Commands/EpicUnrealMCPEditorCommands.h"
#include "Commands/EpicUnrealMCPBlueprintCommands.h"
#include "Commands/EpicUnrealMCPBlueprintGraphCommands.h"
#include "Commands/EpicUnrealMCPMaterialGraphCommands.h"
#include "Commands/EpicUnrealMCPLandscapeCommands.h"
#include "Commands/EpicUnrealMCPGameplayCommands.h"
#include "Commands/EpicUnrealMCPWidgetCommands.h"
#include "Commands/EpicUnrealMCPAICommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"

// Default settings
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557

UEpicUnrealMCPBridge::UEpicUnrealMCPBridge()
{
    EditorCommands = MakeShared<FEpicUnrealMCPEditorCommands>();
    BlueprintCommands = MakeShared<FEpicUnrealMCPBlueprintCommands>();
    BlueprintGraphCommands = MakeShared<FEpicUnrealMCPBlueprintGraphCommands>();
    MaterialGraphCommands = MakeShared<FEpicUnrealMCPMaterialGraphCommands>();
    LandscapeCommands = MakeShared<FEpicUnrealMCPLandscapeCommands>();
    GameplayCommands = MakeShared<FEpicUnrealMCPGameplayCommands>();
    WidgetCommands = MakeShared<FEpicUnrealMCPWidgetCommands>();
    AICommands = MakeShared<FEpicUnrealMCPAICommands>();
}

UEpicUnrealMCPBridge::~UEpicUnrealMCPBridge()
{
    EditorCommands.Reset();
    BlueprintCommands.Reset();
    BlueprintGraphCommands.Reset();
    MaterialGraphCommands.Reset();
    GameplayCommands.Reset();
    WidgetCommands.Reset();
    AICommands.Reset();
}

// Initialize subsystem
void UEpicUnrealMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Display, TEXT("EpicUnrealMCPBridge: Initializing"));
    
    bIsRunning = false;
    ListenerSocket = nullptr;
    ConnectionSocket = nullptr;
    ServerThread = nullptr;
    Port = MCP_SERVER_PORT;
    FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

    // Start the server automatically
    StartServer();
}

// Clean up resources when subsystem is destroyed
void UEpicUnrealMCPBridge::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("EpicUnrealMCPBridge: Shutting down"));
    StopServer();
}

// Start the MCP server
void UEpicUnrealMCPBridge::StartServer()
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("EpicUnrealMCPBridge: Server is already running"));
        return;
    }

    // Create socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("EpicUnrealMCPBridge: Failed to get socket subsystem"));
        return;
    }

    // Create listener socket
    TSharedPtr<FSocket> NewListenerSocket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealMCPListener"), false));
    if (!NewListenerSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("EpicUnrealMCPBridge: Failed to create listener socket"));
        return;
    }

    // Allow address reuse for quick restarts
    NewListenerSocket->SetReuseAddr(true);
    NewListenerSocket->SetNonBlocking(true);

    // Bind to address
    FIPv4Endpoint Endpoint(ServerAddress, Port);
    if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogTemp, Error, TEXT("EpicUnrealMCPBridge: Failed to bind listener socket to %s:%d"), *ServerAddress.ToString(), Port);
        return;
    }

    // Start listening
    if (!NewListenerSocket->Listen(5))
    {
        UE_LOG(LogTemp, Error, TEXT("EpicUnrealMCPBridge: Failed to start listening"));
        return;
    }

    ListenerSocket = NewListenerSocket;
    bIsRunning = true;
    UE_LOG(LogTemp, Display, TEXT("EpicUnrealMCPBridge: Server started on %s:%d"), *ServerAddress.ToString(), Port);

    // Start server thread
    ServerThread = FRunnableThread::Create(
        new FMCPServerRunnable(this, ListenerSocket),
        TEXT("UnrealMCPServerThread"),
        0, TPri_Normal
    );

    if (!ServerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("EpicUnrealMCPBridge: Failed to create server thread"));
        StopServer();
        return;
    }
}

// Stop the MCP server
void UEpicUnrealMCPBridge::StopServer()
{
    if (!bIsRunning)
    {
        return;
    }

    bIsRunning = false;

    // Clean up thread
    if (ServerThread)
    {
        ServerThread->Kill(true);
        delete ServerThread;
        ServerThread = nullptr;
    }

    // Close sockets
    if (ConnectionSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket.Get());
        ConnectionSocket.Reset();
    }

    if (ListenerSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
        ListenerSocket.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("EpicUnrealMCPBridge: Server stopped"));
}

// Execute a command received from a client
FString UEpicUnrealMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Display, TEXT("EpicUnrealMCPBridge: Executing command: %s"), *CommandType);
    
    // Use TSharedPtr for the promise so the lambda is copyable (required by FTickerDelegate)
    TSharedPtr<TPromise<FString>> Promise = MakeShared<TPromise<FString>>();
    TFuture<FString> Future = Promise->GetFuture();

    // === Special handling for take_screenshot ===
    // Screenshot uses a 2-phase ticker to avoid a deadlock:
    //   Phase 0: Spawn SceneCapture2D + CaptureScene() (enqueues render commands)
    //   Phase 1: ReadPixels() + PNG encode + save (requires render commands to be complete)
    // Doing both in the same tick can deadlock because ReadPixels() internally calls
    // FlushRenderingCommands() while the CaptureScene render commands are still pending.
    if (CommandType == TEXT("take_screenshot"))
    {
        struct FScreenshotState
        {
            int32 Phase = 0;
            int32 FrameCount = 0;
            ASceneCapture2D* CaptureActor = nullptr;
            UTextureRenderTarget2D* RenderTarget = nullptr;
            FString FilePath;
            int32 Width = 0;
            int32 Height = 0;
        };
        auto State = MakeShared<FScreenshotState>();

        FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([this, Params, Promise, State](float DeltaTime) -> bool
        {
            if (State->Phase == 0)
            {
                // === Phase 0: Setup + CaptureScene ===
                UE_LOG(LogTemp, Display, TEXT("Screenshot Phase 0: Setting up SceneCapture2D"));

                if (!Params->TryGetStringField(TEXT("file_path"), State->FilePath))
                {
                    State->FilePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("MCP_Screenshot.png");
                }

                State->Width = 960;
                State->Height = 540;
                if (Params->HasField(TEXT("width")))
                {
                    State->Width = FMath::Clamp(static_cast<int32>(Params->GetNumberField(TEXT("width"))), 320, 3840);
                }
                if (Params->HasField(TEXT("height")))
                {
                    State->Height = FMath::Clamp(static_cast<int32>(Params->GetNumberField(TEXT("height"))), 240, 2160);
                }

                IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
                PlatformFile.CreateDirectoryTree(*FPaths::GetPath(State->FilePath));

                // Get viewport camera
                FVector CameraLocation = FVector::ZeroVector;
                FRotator CameraRotation = FRotator::ZeroRotator;
                float CameraFOV = 90.0f;
                bool bFoundCamera = false;
                FLevelEditorViewportClient* UsedClient = nullptr;

                if (GEditor)
                {
                    const TArray<FLevelEditorViewportClient*>& LevelViewports = GEditor->GetLevelViewportClients();
                    for (FLevelEditorViewportClient* VC : LevelViewports)
                    {
                        if (VC && VC->IsPerspective())
                        {
                            CameraLocation = VC->GetViewLocation();
                            CameraRotation = VC->GetViewRotation();
                            CameraFOV = VC->ViewFOV;
                            UsedClient = VC;
                            bFoundCamera = true;
                            break;
                        }
                    }
                    if (!bFoundCamera)
                    {
                        for (FLevelEditorViewportClient* VC : LevelViewports)
                        {
                            if (VC)
                            {
                                CameraLocation = VC->GetViewLocation();
                                CameraRotation = VC->GetViewRotation();
                                CameraFOV = VC->ViewFOV;
                                UsedClient = VC;
                                bFoundCamera = true;
                                break;
                            }
                        }
                    }
                }

                if (!bFoundCamera)
                {
                    TSharedPtr<FJsonObject> Err = MakeShareable(new FJsonObject);
                    Err->SetStringField(TEXT("status"), TEXT("error"));
                    Err->SetStringField(TEXT("error"), TEXT("No editor viewport camera found"));
                    FString ErrStr;
                    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&ErrStr);
                    FJsonSerializer::Serialize(Err.ToSharedRef(), W);
                    Promise->SetValue(ErrStr);
                    return false;
                }

                UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
                if (!World)
                {
                    TSharedPtr<FJsonObject> Err = MakeShareable(new FJsonObject);
                    Err->SetStringField(TEXT("status"), TEXT("error"));
                    Err->SetStringField(TEXT("error"), TEXT("No editor world available"));
                    FString ErrStr;
                    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&ErrStr);
                    FJsonSerializer::Serialize(Err.ToSharedRef(), W);
                    Promise->SetValue(ErrStr);
                    return false;
                }

                // Create render target
                State->RenderTarget = NewObject<UTextureRenderTarget2D>();
                State->RenderTarget->AddToRoot(); // Prevent GC between ticks
                State->RenderTarget->InitCustomFormat(State->Width, State->Height, PF_B8G8R8A8, true);
                State->RenderTarget->UpdateResourceImmediate(false);

                // Spawn capture actor
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
                SpawnParams.ObjectFlags = RF_Transient;

                State->CaptureActor = World->SpawnActor<ASceneCapture2D>(
                    CameraLocation, CameraRotation, SpawnParams);

                if (!State->CaptureActor)
                {
                    State->RenderTarget->RemoveFromRoot();
                    TSharedPtr<FJsonObject> Err = MakeShareable(new FJsonObject);
                    Err->SetStringField(TEXT("status"), TEXT("error"));
                    Err->SetStringField(TEXT("error"), TEXT("Failed to spawn SceneCapture2D actor"));
                    FString ErrStr;
                    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&ErrStr);
                    FJsonSerializer::Serialize(Err.ToSharedRef(), W);
                    Promise->SetValue(ErrStr);
                    return false;
                }

                // Configure capture
                USceneCaptureComponent2D* CC = State->CaptureActor->GetCaptureComponent2D();
                CC->TextureTarget = State->RenderTarget;
                CC->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
                CC->bCaptureEveryFrame = false;
                CC->bCaptureOnMovement = false;
                CC->bAlwaysPersistRenderingState = true;
                CC->FOVAngle = CameraFOV;
                CC->HiddenActors.Add(State->CaptureActor);

                if (UsedClient)
                {
                    FExposureSettings ExpSettings = UsedClient->ExposureSettings;
                    if (ExpSettings.bFixed)
                    {
                        CC->PostProcessBlendWeight = 1.0f;
                        CC->PostProcessSettings.bOverride_AutoExposureMethod = true;
                        CC->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
                        CC->PostProcessSettings.bOverride_AutoExposureBias = true;
                        CC->PostProcessSettings.AutoExposureBias = ExpSettings.FixedEV100;
                    }
                }

                // Enqueue capture render commands — these will be processed by
                // the render thread BETWEEN this tick and the next tick.
                CC->CaptureScene();

                UE_LOG(LogTemp, Display, TEXT("Screenshot Phase 0 complete: CaptureScene enqueued, waiting for render"));
                State->Phase = 1;
                State->FrameCount = 0;
                return true; // Come back next tick
            }
            else if (State->Phase == 1)
            {
                // Wait 2 extra frames to ensure render thread has fully
                // processed the CaptureScene commands.
                State->FrameCount++;
                if (State->FrameCount < 3)
                {
                    return true; // Wait more frames
                }

                // === Phase 1: ReadPixels + encode + save + cleanup ===
                UE_LOG(LogTemp, Display, TEXT("Screenshot Phase 1: ReadPixels after %d frames"), State->FrameCount);

                TArray<FColor> Bitmap;
                bool bReadOK = false;

                FTextureRenderTargetResource* RTResource = State->RenderTarget->GameThread_GetRenderTargetResource();
                if (RTResource)
                {
                    bReadOK = RTResource->ReadPixels(Bitmap);
                }

                // Cleanup capture actor
                UEditorActorSubsystem* ActorSub = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
                if (ActorSub && State->CaptureActor)
                {
                    ActorSub->DestroyActor(State->CaptureActor);
                }
                State->CaptureActor = nullptr;

                // Remove GC root from render target
                State->RenderTarget->RemoveFromRoot();

                if (!bReadOK)
                {
                    TSharedPtr<FJsonObject> Err = MakeShareable(new FJsonObject);
                    Err->SetStringField(TEXT("status"), TEXT("error"));
                    Err->SetStringField(TEXT("error"), TEXT("Failed to read pixels from render target"));
                    FString ErrStr;
                    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&ErrStr);
                    FJsonSerializer::Serialize(Err.ToSharedRef(), W);
                    Promise->SetValue(ErrStr);
                    return false;
                }

                // Encode to PNG
                IImageWrapperModule& IWM = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
                TSharedPtr<IImageWrapper> ImgWrap = IWM.CreateImageWrapper(EImageFormat::PNG);
                if (!ImgWrap.IsValid() ||
                    !ImgWrap->SetRaw(Bitmap.GetData(), Bitmap.Num() * sizeof(FColor),
                                     State->Width, State->Height, ERGBFormat::BGRA, 8))
                {
                    TSharedPtr<FJsonObject> Err = MakeShareable(new FJsonObject);
                    Err->SetStringField(TEXT("status"), TEXT("error"));
                    Err->SetStringField(TEXT("error"), TEXT("PNG encoding failed"));
                    FString ErrStr;
                    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&ErrStr);
                    FJsonSerializer::Serialize(Err.ToSharedRef(), W);
                    Promise->SetValue(ErrStr);
                    return false;
                }

                TArray64<uint8> PNGData = ImgWrap->GetCompressed();
                if (PNGData.Num() == 0 || !FFileHelper::SaveArrayToFile(PNGData, *State->FilePath))
                {
                    TSharedPtr<FJsonObject> Err = MakeShareable(new FJsonObject);
                    Err->SetStringField(TEXT("status"), TEXT("error"));
                    Err->SetStringField(TEXT("error"), FString::Printf(TEXT("Failed to save screenshot to: %s"), *State->FilePath));
                    FString ErrStr;
                    TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&ErrStr);
                    FJsonSerializer::Serialize(Err.ToSharedRef(), W);
                    Promise->SetValue(ErrStr);
                    return false;
                }

                FString AbsPath = FPaths::ConvertRelativePathToFull(State->FilePath);

                TSharedPtr<FJsonObject> Resp = MakeShareable(new FJsonObject);
                Resp->SetStringField(TEXT("status"), TEXT("success"));
                TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
                Result->SetBoolField(TEXT("success"), true);
                Result->SetStringField(TEXT("file_path"), AbsPath);
                Result->SetNumberField(TEXT("width"), State->Width);
                Result->SetNumberField(TEXT("height"), State->Height);
                Result->SetStringField(TEXT("message"), FString::Printf(
                    TEXT("Screenshot saved: %dx%d to %s"), State->Width, State->Height, *AbsPath));
                Resp->SetObjectField(TEXT("result"), Result);

                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(Resp.ToSharedRef(), Writer);
                Promise->SetValue(ResultString);

                UE_LOG(LogTemp, Display, TEXT("Screenshot Phase 1 complete: saved %dx%d to %s"),
                    State->Width, State->Height, *AbsPath);
                return false; // Done
            }

            return false;
        }));

        return Future.Get();
    }

    // Schedule execution during the next engine tick via FTSTicker.
    // This runs on the game thread during the normal tick loop, NOT inside the task graph's
    // ProcessTasksUntilIdle. This is critical for heavy commands like import_mesh that
    // internally use the task graph (Nanite building, etc.) - running them inside
    // AsyncTask(GameThread) causes a TaskGraph RecursionGuard assertion crash.
    FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateLambda([this, CommandType, Params, Promise](float DeltaTime) -> bool
    {
        TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);
        
        try
        {
            TSharedPtr<FJsonObject> ResultJson;
            
            if (CommandType == TEXT("ping"))
            {
                ResultJson = MakeShareable(new FJsonObject);
                ResultJson->SetStringField(TEXT("message"), TEXT("pong"));
            }
            // Editor Commands (including actor manipulation)
            else if (CommandType == TEXT("get_actors_in_level") ||
                     CommandType == TEXT("find_actors_by_name") ||
                     CommandType == TEXT("spawn_actor") ||
                     CommandType == TEXT("delete_actor") ||
                     CommandType == TEXT("set_actor_transform") ||
                     CommandType == TEXT("spawn_blueprint_actor") ||
                     CommandType == TEXT("set_actor_property") ||
                     CommandType == TEXT("get_actor_properties") ||
                     CommandType == TEXT("create_material") ||
                     CommandType == TEXT("create_material_instance") ||
                     CommandType == TEXT("set_material_instance_parameter") ||
                     CommandType == TEXT("import_texture") ||
                     CommandType == TEXT("set_texture_properties") ||
                     CommandType == TEXT("create_pbr_material") ||
                     CommandType == TEXT("create_landscape_material") ||
                     CommandType == TEXT("import_mesh") ||
                     CommandType == TEXT("list_assets") ||
                     CommandType == TEXT("does_asset_exist") ||
                     CommandType == TEXT("get_asset_info") ||
                     CommandType == TEXT("get_height_at_location") ||
                     CommandType == TEXT("snap_actor_to_ground") ||
                     CommandType == TEXT("scatter_meshes_on_landscape") ||
                     CommandType == TEXT("take_screenshot") ||
                     CommandType == TEXT("get_material_info") ||
                     CommandType == TEXT("focus_viewport_on_actor") ||
                     CommandType == TEXT("get_texture_info") ||
                     CommandType == TEXT("delete_actors_by_pattern") ||
                     CommandType == TEXT("import_skeletal_mesh") ||
                     CommandType == TEXT("import_animation") ||
                     CommandType == TEXT("delete_asset") ||
                     CommandType == TEXT("set_nanite_enabled") ||
                     CommandType == TEXT("scatter_foliage") ||
                     CommandType == TEXT("import_sound") ||
                     CommandType == TEXT("add_anim_notify") ||
                     CommandType == TEXT("get_editor_log"))
            {
                ResultJson = EditorCommands->HandleCommand(CommandType, Params);
            }
            // Blueprint Commands
            else if (CommandType == TEXT("create_blueprint") ||
                     CommandType == TEXT("add_component_to_blueprint") ||
                     CommandType == TEXT("set_physics_properties") ||
                     CommandType == TEXT("compile_blueprint") ||
                     CommandType == TEXT("set_static_mesh_properties") ||
                     CommandType == TEXT("set_mesh_material_color") ||
                     CommandType == TEXT("get_available_materials") ||
                     CommandType == TEXT("apply_material_to_actor") ||
                     CommandType == TEXT("set_mesh_asset_material") ||
                     CommandType == TEXT("apply_material_to_blueprint") ||
                     CommandType == TEXT("get_actor_material_info") ||
                     CommandType == TEXT("get_blueprint_material_info") ||
                     CommandType == TEXT("read_blueprint_content") ||
                     CommandType == TEXT("analyze_blueprint_graph") ||
                     CommandType == TEXT("get_blueprint_variable_details") ||
                     CommandType == TEXT("get_blueprint_function_details") ||
                     CommandType == TEXT("create_character_blueprint") ||
                     CommandType == TEXT("create_anim_blueprint") ||
                     CommandType == TEXT("setup_locomotion_state_machine") ||
                     CommandType == TEXT("setup_blendspace_locomotion") ||
                     CommandType == TEXT("set_character_properties") ||
                     CommandType == TEXT("set_anim_sequence_root_motion") ||
                     CommandType == TEXT("set_anim_state_always_reset_on_entry") ||
                     CommandType == TEXT("set_state_machine_max_transitions_per_frame"))
            {
                ResultJson = BlueprintCommands->HandleCommand(CommandType, Params);
            }
            // Blueprint Graph Commands
            else if (CommandType == TEXT("add_blueprint_node") ||
                     CommandType == TEXT("connect_nodes") ||
                     CommandType == TEXT("create_variable") ||
                     CommandType == TEXT("set_blueprint_variable_properties") ||
                     CommandType == TEXT("add_event_node") ||
                     CommandType == TEXT("delete_node") ||
                     CommandType == TEXT("set_node_property") ||
                     CommandType == TEXT("create_function") ||
                     CommandType == TEXT("add_function_input") ||
                     CommandType == TEXT("add_function_output") ||
                     CommandType == TEXT("delete_function") ||
                     CommandType == TEXT("rename_function") ||
                     CommandType == TEXT("add_enhanced_input_action_event") ||
                     CommandType == TEXT("create_input_action") ||
                     CommandType == TEXT("add_input_mapping"))
            {
                ResultJson = BlueprintGraphCommands->HandleCommand(CommandType, Params);
            }
            // Material Graph Commands
            else if (CommandType == TEXT("create_material_asset") ||
                     CommandType == TEXT("get_material_graph") ||
                     CommandType == TEXT("add_material_expression") ||
                     CommandType == TEXT("connect_material_expressions") ||
                     CommandType == TEXT("connect_to_material_output") ||
                     CommandType == TEXT("set_material_expression_property") ||
                     CommandType == TEXT("delete_material_expression") ||
                     CommandType == TEXT("recompile_material") ||
                     CommandType == TEXT("configure_landscape_layer_blend"))
            {
                ResultJson = MaterialGraphCommands->HandleCommand(CommandType, Params);
            }
            // Landscape Commands
            else if (CommandType == TEXT("get_landscape_info") ||
                     CommandType == TEXT("sculpt_landscape") ||
                     CommandType == TEXT("smooth_landscape") ||
                     CommandType == TEXT("flatten_landscape") ||
                     CommandType == TEXT("paint_landscape_layer") ||
                     CommandType == TEXT("get_landscape_layers") ||
                     CommandType == TEXT("set_landscape_material") ||
                     CommandType == TEXT("create_landscape_layer") ||
                     CommandType == TEXT("add_layer_to_landscape"))
            {
                ResultJson = LandscapeCommands->HandleCommand(CommandType, Params);
            }
            // Gameplay Commands (game mode, montage, impulse, post-process effects, niagara)
            else if (CommandType == TEXT("set_game_mode_default_pawn") ||
                     CommandType == TEXT("create_anim_montage") ||
                     CommandType == TEXT("play_montage_on_actor") ||
                     CommandType == TEXT("apply_impulse") ||
                     CommandType == TEXT("trigger_post_process_effect") ||
                     CommandType == TEXT("spawn_niagara_system") ||
                     CommandType == TEXT("create_niagara_system") ||
                     CommandType == TEXT("set_niagara_parameter") ||
                     CommandType == TEXT("create_atmospheric_fx") ||
                     CommandType == TEXT("set_skeletal_animation"))
            {
                ResultJson = GameplayCommands->HandleCommand(CommandType, Params);
            }
            // Widget Commands (UMG widget blueprints, viewport display, properties)
            else if (CommandType == TEXT("create_widget_blueprint") ||
                     CommandType == TEXT("add_widget_to_viewport") ||
                     CommandType == TEXT("set_widget_property"))
            {
                ResultJson = WidgetCommands->HandleCommand(CommandType, Params);
            }
            // AI Commands (behavior trees, blackboards, tasks, decorators)
            else if (CommandType == TEXT("create_behavior_tree") ||
                     CommandType == TEXT("create_blackboard") ||
                     CommandType == TEXT("add_bt_task") ||
                     CommandType == TEXT("add_bt_decorator") ||
                     CommandType == TEXT("assign_behavior_tree"))
            {
                ResultJson = AICommands->HandleCommand(CommandType, Params);
            }
            else
            {
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), FString::Printf(TEXT("Unknown command: %s"), *CommandType));
                
                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
                Promise->SetValue(ResultString);
                return false;
            }
            
            // Check if the result contains an error
            bool bSuccess = true;
            FString ErrorMessage;
            
            if (ResultJson->HasField(TEXT("success")))
            {
                bSuccess = ResultJson->GetBoolField(TEXT("success"));
                if (!bSuccess && ResultJson->HasField(TEXT("error")))
                {
                    ErrorMessage = ResultJson->GetStringField(TEXT("error"));
                }
            }
            
            if (bSuccess)
            {
                // Set success status and include the result
                ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
            else
            {
                // Set error status and include the error message
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), ErrorMessage);
            }
        }
        catch (const std::exception& e)
        {
            ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
            ResponseJson->SetStringField(TEXT("error"), UTF8_TO_TCHAR(e.what()));
        }
        
        FString ResultString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
        FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
        Promise->SetValue(ResultString);
        return false; // Execute once, don't repeat
    }));

    return Future.Get();
}
