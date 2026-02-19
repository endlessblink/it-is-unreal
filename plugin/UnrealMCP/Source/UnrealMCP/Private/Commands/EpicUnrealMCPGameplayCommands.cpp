#include "Commands/EpicUnrealMCPGameplayCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "EngineUtils.h"

// GameMode / PlayerStart / Blueprint
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EngineUtils.h"

// Animation Montage
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"

// Character / Physics
#include "GameFramework/Character.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Post Process
#include "Engine/PostProcessVolume.h"
#include "Components/PostProcessComponent.h"

// Niagara (requires Niagara module in Build.cs)
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

// Niagara editor utilities for creating systems from templates
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEditorUtilities.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraSpriteRendererProperties.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "NiagaraCommon.h"

FEpicUnrealMCPGameplayCommands::FEpicUnrealMCPGameplayCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("set_game_mode_default_pawn"))
	{
		return HandleSetGameModeDefaultPawn(Params);
	}
	else if (CommandType == TEXT("create_anim_montage"))
	{
		return HandleCreateAnimMontage(Params);
	}
	else if (CommandType == TEXT("play_montage_on_actor"))
	{
		return HandlePlayMontageOnActor(Params);
	}
	else if (CommandType == TEXT("apply_impulse"))
	{
		return HandleApplyImpulse(Params);
	}
	else if (CommandType == TEXT("trigger_post_process_effect"))
	{
		return HandleTriggerPostProcessEffect(Params);
	}
	else if (CommandType == TEXT("spawn_niagara_system"))
	{
		return HandleSpawnNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("create_niagara_system"))
	{
		return HandleCreateNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("set_niagara_parameter"))
	{
		return HandleSetNiagaraParameter(Params);
	}
	else if (CommandType == TEXT("create_atmospheric_fx"))
	{
		return HandleCreateAtmosphericFX(Params);
	}
	else if (CommandType == TEXT("set_skeletal_animation"))
	{
		return HandleSetSkeletalAnimation(Params);
	}

	return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown gameplay command: %s"), *CommandType));
}

// ============================================================================
// 1. HandleSetGameModeDefaultPawn
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleSetGameModeDefaultPawn(const TSharedPtr<FJsonObject>& Params)
{
	// Required: blueprint_path to the character BP
	FString BlueprintPath;
	if (!Params->TryGetStringField(TEXT("blueprint_path"), BlueprintPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_path' parameter"));
	}

	// Load the character blueprint
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(BlueprintPath);
	if (!IsValid(LoadedAsset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load asset at path: %s"), *BlueprintPath));
	}

	UBlueprint* CharacterBP = Cast<UBlueprint>(LoadedAsset);
	if (!CharacterBP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset at '%s' is not a Blueprint"), *BlueprintPath));
	}

	UClass* CharacterClass = CharacterBP->GeneratedClass;
	if (!CharacterClass)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Character Blueprint has no GeneratedClass. Compile the blueprint first."));
	}

	// Optional: game_mode_path
	FString GameModePath;
	Params->TryGetStringField(TEXT("game_mode_path"), GameModePath);

	UBlueprint* GameModeBP = nullptr;
	UPackage* GameModePackage = nullptr;

	if (!GameModePath.IsEmpty())
	{
		// Load existing GameMode BP
		UObject* GMAsset = UEditorAssetLibrary::LoadAsset(GameModePath);
		if (!IsValid(GMAsset))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load GameMode at path: %s"), *GameModePath));
		}

		GameModeBP = Cast<UBlueprint>(GMAsset);
		if (!GameModeBP)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Asset at '%s' is not a Blueprint"), *GameModePath));
		}

		GameModePackage = GameModeBP->GetPackage();
	}
	else
	{
		// Create a new GameMode BP at /Game/Blueprints/BP_GameMode
		FString DefaultGMPath = TEXT("/Game/Blueprints/BP_GameMode");
		FString AssetName = TEXT("BP_GameMode");

		// Check if already exists
		if (UEditorAssetLibrary::DoesAssetExist(DefaultGMPath))
		{
			// Load existing
			UObject* ExistingAsset = UEditorAssetLibrary::LoadAsset(DefaultGMPath);
			GameModeBP = Cast<UBlueprint>(ExistingAsset);
			if (!GameModeBP)
			{
				return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Asset at /Game/Blueprints/BP_GameMode exists but is not a Blueprint"));
			}
			GameModePackage = GameModeBP->GetPackage();
			GameModePath = DefaultGMPath;
		}
		else
		{
			// Create new GameMode Blueprint
			UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
			Factory->ParentClass = AGameModeBase::StaticClass();

			GameModePackage = CreatePackage(*DefaultGMPath);
			GameModeBP = Cast<UBlueprint>(Factory->FactoryCreateNew(
				UBlueprint::StaticClass(),
				GameModePackage,
				*AssetName,
				RF_Standalone | RF_Public,
				nullptr,
				GWarn
			));

			if (!GameModeBP)
			{
				return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create GameMode Blueprint"));
			}

			FAssetRegistryModule::AssetCreated(GameModeBP);
			GameModePackage->MarkPackageDirty();
			GameModePath = DefaultGMPath;

			UE_LOG(LogTemp, Log, TEXT("Created new GameMode Blueprint at %s"), *DefaultGMPath);
		}
	}

	// Compile the GameMode BP first so the CDO is available
	FKismetEditorUtilities::CompileBlueprint(GameModeBP);

	// Get the CDO and set DefaultPawnClass
	UClass* GameModeClass = GameModeBP->GeneratedClass;
	if (!GameModeClass)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("GameMode Blueprint has no GeneratedClass after compile"));
	}

	AGameModeBase* GameModeCDO = GameModeClass->GetDefaultObject<AGameModeBase>();
	if (!GameModeCDO)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get GameMode CDO"));
	}

	GameModeCDO->DefaultPawnClass = CharacterClass;
	UE_LOG(LogTemp, Log, TEXT("Set DefaultPawnClass to '%s' on GameMode '%s'"),
		*CharacterClass->GetName(), *GameModeClass->GetName());

	// Recompile after CDO modification
	FKismetEditorUtilities::CompileBlueprint(GameModeBP);
	GameModePackage->MarkPackageDirty();

	// Save the GameMode package
	FString PackageFilename = FPackageName::LongPackageNameToFilename(GameModePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(GameModePackage, GameModeBP, *PackageFilename, SaveArgs);

	// Optional: create_player_start (default true)
	bool bCreatePlayerStart = true;
	Params->TryGetBoolField(TEXT("create_player_start"), bCreatePlayerStart);

	bool bPlayerStartSpawned = false;

	if (bCreatePlayerStart)
	{
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
		}

		// Get optional player_start_location (default 0,0,100)
		FVector PlayerStartLocation(0.0f, 0.0f, 100.0f);
		if (Params->HasField(TEXT("player_start_location")))
		{
			PlayerStartLocation = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("player_start_location"));
		}

		// Check if a PlayerStart already exists
		TArray<AActor*> ExistingStarts;
		UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingStarts);

		if (ExistingStarts.Num() == 0)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Name = FName(TEXT("PlayerStart_MCP"));
			SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

			APlayerStart* NewPlayerStart = World->SpawnActor<APlayerStart>(
				APlayerStart::StaticClass(),
				PlayerStartLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);

			if (IsValid(NewPlayerStart))
			{
				NewPlayerStart->SetFlags(RF_Transactional);
				NewPlayerStart->GetRootComponent()->SetFlags(RF_Transactional);

				// Mark OFPA package dirty for persistence
				if (UPackage* ActorPackage = NewPlayerStart->GetExternalPackage())
				{
					ActorPackage->SetDirtyFlag(true);
				}

				bPlayerStartSpawned = true;
				UE_LOG(LogTemp, Log, TEXT("Spawned PlayerStart at (%f, %f, %f)"),
					PlayerStartLocation.X, PlayerStartLocation.Y, PlayerStartLocation.Z);
			}
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("PlayerStart already exists in level, skipping spawn"));
		}
	}

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("game_mode_path"), GameModePath);
	ResultObj->SetStringField(TEXT("pawn_class"), CharacterClass->GetName());
	ResultObj->SetBoolField(TEXT("player_start_spawned"), bPlayerStartSpawned);

	if (GameModeBP->GeneratedClass)
	{
		ResultObj->SetStringField(TEXT("game_mode_class"), GameModeBP->GeneratedClass->GetPathName());
	}

	return ResultObj;
}

// ============================================================================
// 2. HandleCreateAnimMontage
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleCreateAnimMontage(const TSharedPtr<FJsonObject>& Params)
{
	// Required: animation_path
	FString AnimationPath;
	if (!Params->TryGetStringField(TEXT("animation_path"), AnimationPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'animation_path' parameter"));
	}

	// Required: montage_name
	FString MontageName;
	if (!Params->TryGetStringField(TEXT("montage_name"), MontageName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'montage_name' parameter"));
	}

	// Load the AnimSequence
	UAnimSequence* AnimSeq = Cast<UAnimSequence>(UEditorAssetLibrary::LoadAsset(AnimationPath));
	if (!IsValid(AnimSeq))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimSequence at path: %s"), *AnimationPath));
	}

	USkeleton* Skeleton = AnimSeq->GetSkeleton();
	if (!IsValid(Skeleton))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("AnimSequence has no valid Skeleton"));
	}

	// Determine destination path
	FString DestinationPath;
	if (!Params->TryGetStringField(TEXT("destination_path"), DestinationPath))
	{
		// Extract directory from animation_path
		DestinationPath = FPaths::GetPath(AnimationPath);
		if (DestinationPath.IsEmpty())
		{
			DestinationPath = TEXT("/Game/Animations");
		}
	}

	// Optional: slot_name
	FString SlotName = TEXT("DefaultGroup.DefaultSlot");
	Params->TryGetStringField(TEXT("slot_name"), SlotName);

	// Create package
	FString FullMontageAssetPath = DestinationPath / MontageName;
	UPackage* Package = CreatePackage(*FullMontageAssetPath);
	if (!Package)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create package at: %s"), *FullMontageAssetPath));
	}

	// Create the montage object
	UAnimMontage* Montage = NewObject<UAnimMontage>(Package, FName(*MontageName), RF_Public | RF_Standalone);
	if (!Montage)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UAnimMontage object"));
	}

	// Set skeleton
	Montage->SetSkeleton(Skeleton);

	// Get the sequence length
	double SequenceLength = AnimSeq->GetPlayLength();

	// Setup a default SlotAnimTrack with the AnimSequence
	// UAnimMontage::SlotAnimTracks is the array of FSlotAnimationTrack
	if (Montage->SlotAnimTracks.Num() == 0)
	{
		Montage->SlotAnimTracks.AddDefaulted(1);
	}

	FSlotAnimationTrack& SlotTrack = Montage->SlotAnimTracks[0];
	SlotTrack.SlotName = FName(*SlotName);

	// Add an FAnimSegment referencing the AnimSequence
	FAnimSegment NewSegment;
	NewSegment.SetAnimReference(AnimSeq);
	NewSegment.AnimStartTime = 0.0f;
	NewSegment.AnimEndTime = SequenceLength;
	NewSegment.AnimPlayRate = 1.0f;
	NewSegment.StartPos = 0.0f;

	SlotTrack.AnimTrack.AnimSegments.Add(NewSegment);

	// Update the montage's overall length from its segments
	Montage->CalculateSequenceLength();

	// Create a default composite section spanning the whole montage
	if (Montage->CompositeSections.Num() == 0)
	{
		FCompositeSection DefaultSection;
		DefaultSection.SectionName = FName(TEXT("Default"));
		DefaultSection.SetTime(0.0f);
		DefaultSection.NextSectionName = NAME_None;
		Montage->CompositeSections.Add(DefaultSection);
	}

	// Notify asset registry and save
	FAssetRegistryModule::AssetCreated(Montage);
	Package->MarkPackageDirty();

	FString PackageFilename = FPackageName::LongPackageNameToFilename(FullMontageAssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, Montage, *PackageFilename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Created AnimMontage '%s' at '%s' (duration: %.2f, slot: %s)"),
		*MontageName, *FullMontageAssetPath, SequenceLength, *SlotName);

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("montage_path"), FullMontageAssetPath);
	ResultObj->SetStringField(TEXT("montage_name"), MontageName);
	ResultObj->SetNumberField(TEXT("duration"), SequenceLength);
	ResultObj->SetStringField(TEXT("slot_name"), SlotName);
	ResultObj->SetStringField(TEXT("skeleton"), Skeleton->GetPathName());

	return ResultObj;
}

// ============================================================================
// 3. HandlePlayMontageOnActor
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandlePlayMontageOnActor(const TSharedPtr<FJsonObject>& Params)
{
	// Required: actor_name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	// Required: montage_path
	FString MontagePath;
	if (!Params->TryGetStringField(TEXT("montage_path"), MontagePath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'montage_path' parameter"));
	}

	// Check if PIE is active
	UWorld* PIEWorld = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			PIEWorld = Context.World();
			break;
		}
	}

	if (!PIEWorld)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("PlayMontage only works during Play-In-Editor (PIE). Start PIE first, then call this command."));
	}

	// Find actor in the PIE world
	AActor* FoundActor = FEpicUnrealMCPCommonUtils::FindActorByName(PIEWorld, ActorName);
	if (!IsValid(FoundActor))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found in PIE world: %s"), *ActorName));
	}

	ACharacter* Character = Cast<ACharacter>(FoundActor);
	if (!Character)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' is not a Character"), *ActorName));
	}

	// Load montage
	UAnimMontage* Montage = Cast<UAnimMontage>(UEditorAssetLibrary::LoadAsset(MontagePath));
	if (!IsValid(Montage))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load AnimMontage at path: %s"), *MontagePath));
	}

	// Optional: play_rate (default 1.0)
	double PlayRate = 1.0;
	Params->TryGetNumberField(TEXT("play_rate"), PlayRate);

	// Optional: start_section
	FString StartSectionStr;
	Params->TryGetStringField(TEXT("start_section"), StartSectionStr);
	FName StartSection = StartSectionStr.IsEmpty() ? NAME_None : FName(*StartSectionStr);

	// Play the montage
	float Duration = Character->PlayAnimMontage(Montage, (float)PlayRate, StartSection);

	UE_LOG(LogTemp, Log, TEXT("PlayAnimMontage '%s' on '%s' - rate: %.2f, duration: %.2f"),
		*Montage->GetName(), *ActorName, PlayRate, Duration);

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("montage_name"), Montage->GetName());
	ResultObj->SetNumberField(TEXT("play_duration"), Duration);
	ResultObj->SetNumberField(TEXT("play_rate"), PlayRate);
	ResultObj->SetStringField(TEXT("actor"), ActorName);

	return ResultObj;
}

// ============================================================================
// 4. HandleApplyImpulse
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleApplyImpulse(const TSharedPtr<FJsonObject>& Params)
{
	// Required: actor_name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	// Required: direction (vector)
	if (!Params->HasField(TEXT("direction")))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'direction' parameter"));
	}
	FVector Direction = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("direction"));

	// Required: magnitude
	double Magnitude = 0.0;
	if (!Params->TryGetNumberField(TEXT("magnitude"), Magnitude))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'magnitude' parameter"));
	}

	// Optional: enable_ragdoll (default false)
	bool bEnableRagdoll = false;
	Params->TryGetBoolField(TEXT("enable_ragdoll"), bEnableRagdoll);

	// Optional: component_name
	FString ComponentName;
	Params->TryGetStringField(TEXT("component_name"), ComponentName);

	// Try to find actor in PIE world first, then fall back to editor world
	UWorld* TargetWorld = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			TargetWorld = Context.World();
			break;
		}
	}

	bool bInPIE = (TargetWorld != nullptr);

	if (!TargetWorld)
	{
		TargetWorld = GEditor->GetEditorWorldContext().World();
	}

	if (!TargetWorld)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get world"));
	}

	// Find actor
	AActor* FoundActor = FEpicUnrealMCPCommonUtils::FindActorByName(TargetWorld, ActorName);
	if (!IsValid(FoundActor))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	FVector ImpulseVector = Direction.GetSafeNormal() * (float)Magnitude;

	if (bEnableRagdoll)
	{
		// Ragdoll mode: only works on Characters
		ACharacter* Character = Cast<ACharacter>(FoundActor);
		if (!Character)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' is not a Character (required for ragdoll)"), *ActorName));
		}

		USkeletalMeshComponent* MeshComp = Character->GetMesh();
		if (!IsValid(MeshComp))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Character has no valid SkeletalMeshComponent"));
		}

		MeshComp->SetAllBodiesSimulatePhysics(true);
		MeshComp->SetSimulatePhysics(true);
		MeshComp->AddImpulse(ImpulseVector);

		UE_LOG(LogTemp, Log, TEXT("Applied ragdoll impulse (%f, %f, %f) to '%s'"),
			ImpulseVector.X, ImpulseVector.Y, ImpulseVector.Z, *ActorName);
	}
	else
	{
		// Standard impulse mode
		UPrimitiveComponent* PrimComp = nullptr;

		if (!ComponentName.IsEmpty())
		{
			// Find component by name
			TArray<UActorComponent*> Components;
			FoundActor->GetComponents(Components);
			for (UActorComponent* Comp : Components)
			{
				if (Comp && Comp->GetName() == ComponentName)
				{
					PrimComp = Cast<UPrimitiveComponent>(Comp);
					break;
				}
			}

			if (!PrimComp)
			{
				return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
					TEXT("Component '%s' not found or is not a PrimitiveComponent on actor '%s'"),
					*ComponentName, *ActorName));
			}
		}
		else
		{
			// Use root component
			PrimComp = Cast<UPrimitiveComponent>(FoundActor->GetRootComponent());
			if (!PrimComp)
			{
				// Try first primitive component
				PrimComp = FoundActor->FindComponentByClass<UPrimitiveComponent>();
			}
		}

		if (!IsValid(PrimComp))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("No PrimitiveComponent found on actor '%s'"), *ActorName));
		}

		PrimComp->SetSimulatePhysics(true);
		PrimComp->AddImpulse(ImpulseVector);

		UE_LOG(LogTemp, Log, TEXT("Applied impulse (%f, %f, %f) to component '%s' on '%s'"),
			ImpulseVector.X, ImpulseVector.Y, ImpulseVector.Z, *PrimComp->GetName(), *ActorName);
	}

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetBoolField(TEXT("ragdoll"), bEnableRagdoll);

	TArray<TSharedPtr<FJsonValue>> ImpulseArray;
	ImpulseArray.Add(MakeShared<FJsonValueNumber>(ImpulseVector.X));
	ImpulseArray.Add(MakeShared<FJsonValueNumber>(ImpulseVector.Y));
	ImpulseArray.Add(MakeShared<FJsonValueNumber>(ImpulseVector.Z));
	ResultObj->SetArrayField(TEXT("impulse_applied"), ImpulseArray);

	if (!bInPIE)
	{
		ResultObj->SetStringField(TEXT("warning"), TEXT("Physics impulses only take visual effect during Play-In-Editor (PIE). The physics state has been set but will not animate in editor mode."));
	}

	return ResultObj;
}

// ============================================================================
// 5. HandleTriggerPostProcessEffect
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleTriggerPostProcessEffect(const TSharedPtr<FJsonObject>& Params)
{
	// Required: effect_type
	FString EffectType;
	if (!Params->TryGetStringField(TEXT("effect_type"), EffectType))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'effect_type' parameter. Must be one of: red_flash, slow_mo, desaturate, custom"));
	}

	// Optional: duration (default 0.5)
	double Duration = 0.5;
	Params->TryGetNumberField(TEXT("duration"), Duration);

	// Optional: intensity (default 1.0)
	double Intensity = 1.0;
	Params->TryGetNumberField(TEXT("intensity"), Intensity);

	// Validate effect_type
	if (EffectType != TEXT("red_flash") && EffectType != TEXT("slow_mo") &&
		EffectType != TEXT("desaturate") && EffectType != TEXT("custom"))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Invalid effect_type: '%s'. Must be one of: red_flash, slow_mo, desaturate, custom"), *EffectType));
	}

	// Try PIE world first, fall back to editor world
	UWorld* TargetWorld = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::PIE && Context.World())
		{
			TargetWorld = Context.World();
			break;
		}
	}

	bool bInPIE = (TargetWorld != nullptr);

	if (!TargetWorld)
	{
		TargetWorld = GEditor->GetEditorWorldContext().World();
	}

	if (!TargetWorld)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get world"));
	}

	// Handle slow_mo separately (no PPV needed)
	if (EffectType == TEXT("slow_mo"))
	{
		if (!bInPIE)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				TEXT("slow_mo effect only works during Play-In-Editor (PIE). Time dilation has no effect in editor mode."));
		}

		float TargetDilation = FMath::Lerp(1.0f, 0.1f, (float)Intensity);
		UGameplayStatics::SetGlobalTimeDilation(TargetWorld, TargetDilation);

		// Set a timer to restore normal time dilation
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TWeakObjectPtr<UWorld> WeakWorld = TargetWorld;
		TimerDelegate.BindLambda([WeakWorld]()
		{
			if (UWorld* W = WeakWorld.Get())
			{
				UGameplayStatics::SetGlobalTimeDilation(W, 1.0f);
				UE_LOG(LogTemp, Log, TEXT("Restored global time dilation to 1.0"));
			}
		});
		TargetWorld->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, (float)Duration, false);

		UE_LOG(LogTemp, Log, TEXT("Set global time dilation to %.2f for %.2f seconds"), TargetDilation, Duration);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetStringField(TEXT("effect_type"), EffectType);
		ResultObj->SetNumberField(TEXT("time_dilation"), TargetDilation);
		ResultObj->SetNumberField(TEXT("duration"), Duration);
		return ResultObj;
	}

	// Visual effects: spawn an unbound PostProcessVolume
	FActorSpawnParameters SpawnParams;
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	SpawnParams.Name = FName(*FString::Printf(TEXT("PPV_Effect_%s"), *FGuid::NewGuid().ToString()));

	APostProcessVolume* PPV = TargetWorld->SpawnActor<APostProcessVolume>(
		APostProcessVolume::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!IsValid(PPV))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn PostProcessVolume"));
	}

	PPV->bUnbound = true;
	PPV->BlendWeight = 1.0f;

	// Apply effect-specific settings
	if (EffectType == TEXT("red_flash"))
	{
		PPV->Settings.bOverride_SceneColorTint = true;
		PPV->Settings.SceneColorTint = FLinearColor((float)Intensity, 0.0f, 0.0f, 1.0f);
		UE_LOG(LogTemp, Log, TEXT("Applied red_flash effect (intensity: %.2f, duration: %.2f)"), Intensity, Duration);
	}
	else if (EffectType == TEXT("desaturate"))
	{
		PPV->Settings.bOverride_ColorSaturation = true;
		float SatValue = 1.0f - (float)Intensity;
		PPV->Settings.ColorSaturation = FVector4(SatValue, SatValue, SatValue, 1.0f);
		UE_LOG(LogTemp, Log, TEXT("Applied desaturate effect (intensity: %.2f, duration: %.2f)"), Intensity, Duration);
	}
	else if (EffectType == TEXT("custom"))
	{
		// Apply custom_settings if provided
		const TSharedPtr<FJsonObject>* CustomSettings = nullptr;
		if (Params->TryGetObjectField(TEXT("custom_settings"), CustomSettings) && CustomSettings && (*CustomSettings).IsValid())
		{
			// Apply SceneColorTint if provided
			if ((*CustomSettings)->HasField(TEXT("scene_color_tint")))
			{
				PPV->Settings.bOverride_SceneColorTint = true;
				const TSharedPtr<FJsonObject>& TintObj = (*CustomSettings)->GetObjectField(TEXT("scene_color_tint"));
				if (TintObj.IsValid())
				{
					float R = (float)TintObj->GetNumberField(TEXT("R"));
					float G = (float)TintObj->GetNumberField(TEXT("G"));
					float B = (float)TintObj->GetNumberField(TEXT("B"));
					float A = TintObj->HasField(TEXT("A")) ? (float)TintObj->GetNumberField(TEXT("A")) : 1.0f;
					PPV->Settings.SceneColorTint = FLinearColor(R, G, B, A);
				}
			}

			// Apply ColorSaturation if provided
			if ((*CustomSettings)->HasField(TEXT("color_saturation")))
			{
				PPV->Settings.bOverride_ColorSaturation = true;
				FVector SatVec = FEpicUnrealMCPCommonUtils::GetVectorFromJson(*CustomSettings, TEXT("color_saturation"));
				PPV->Settings.ColorSaturation = FVector4(SatVec.X, SatVec.Y, SatVec.Z, 1.0f);
			}

			// Apply ColorContrast if provided
			if ((*CustomSettings)->HasField(TEXT("color_contrast")))
			{
				PPV->Settings.bOverride_ColorContrast = true;
				FVector ConVec = FEpicUnrealMCPCommonUtils::GetVectorFromJson(*CustomSettings, TEXT("color_contrast"));
				PPV->Settings.ColorContrast = FVector4(ConVec.X, ConVec.Y, ConVec.Z, 1.0f);
			}

			// Apply ColorGamma if provided
			if ((*CustomSettings)->HasField(TEXT("color_gamma")))
			{
				PPV->Settings.bOverride_ColorGamma = true;
				FVector GamVec = FEpicUnrealMCPCommonUtils::GetVectorFromJson(*CustomSettings, TEXT("color_gamma"));
				PPV->Settings.ColorGamma = FVector4(GamVec.X, GamVec.Y, GamVec.Z, 1.0f);
			}

			// Apply ColorGain if provided
			if ((*CustomSettings)->HasField(TEXT("color_gain")))
			{
				PPV->Settings.bOverride_ColorGain = true;
				FVector GainVec = FEpicUnrealMCPCommonUtils::GetVectorFromJson(*CustomSettings, TEXT("color_gain"));
				PPV->Settings.ColorGain = FVector4(GainVec.X, GainVec.Y, GainVec.Z, 1.0f);
			}

			// Apply bloom intensity
			double BloomIntensity = 0.0;
			if ((*CustomSettings)->TryGetNumberField(TEXT("bloom_intensity"), BloomIntensity))
			{
				PPV->Settings.bOverride_BloomIntensity = true;
				PPV->Settings.BloomIntensity = (float)BloomIntensity;
			}

			// Apply vignette intensity
			double VignetteIntensity = 0.0;
			if ((*CustomSettings)->TryGetNumberField(TEXT("vignette_intensity"), VignetteIntensity))
			{
				PPV->Settings.bOverride_VignetteIntensity = true;
				PPV->Settings.VignetteIntensity = (float)VignetteIntensity;
			}
		}

		UE_LOG(LogTemp, Log, TEXT("Applied custom post-process effect (duration: %.2f)"), Duration);
	}

	// Set timer to destroy PPV after duration (only meaningful in PIE)
	if (bInPIE)
	{
		FTimerHandle TimerHandle;
		FTimerDelegate TimerDelegate;
		TWeakObjectPtr<APostProcessVolume> WeakPPV = PPV;
		TimerDelegate.BindLambda([WeakPPV]()
		{
			if (APostProcessVolume* StrongPPV = WeakPPV.Get())
			{
				if (IsValid(StrongPPV) && StrongPPV->GetWorld())
				{
					StrongPPV->GetWorld()->DestroyActor(StrongPPV);
					UE_LOG(LogTemp, Log, TEXT("Destroyed temporary post-process effect PPV"));
				}
			}
		});
		TargetWorld->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, (float)Duration, false);
	}

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("effect_type"), EffectType);
	ResultObj->SetNumberField(TEXT("duration"), Duration);
	ResultObj->SetNumberField(TEXT("intensity"), Intensity);
	ResultObj->SetStringField(TEXT("ppv_name"), PPV->GetName());

	if (!bInPIE)
	{
		ResultObj->SetStringField(TEXT("note"), TEXT("Not in PIE mode. The PostProcessVolume has been spawned and will persist in the editor. Timer-based auto-destruction only works during PIE. You may need to delete it manually."));
	}

	return ResultObj;
}

// ============================================================================
// 6. HandleSpawnNiagaraSystem
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleSpawnNiagaraSystem(const TSharedPtr<FJsonObject>& Params)
{
	// Required: actor_name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	// Required: system_path
	FString SystemPath;
	if (!Params->TryGetStringField(TEXT("system_path"), SystemPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_path' parameter"));
	}

	// Get world
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
	}

	// Load Niagara system
	UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(UEditorAssetLibrary::LoadAsset(SystemPath));
	if (!IsValid(NiagaraSystem))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load NiagaraSystem at path: %s"), *SystemPath));
	}

	// Optional location, rotation, scale
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

	// Optional: auto_activate (default true)
	bool bAutoActivate = true;
	Params->TryGetBoolField(TEXT("auto_activate"), bAutoActivate);

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

	// Spawn the Niagara actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(*ActorName);
	SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

	ANiagaraActor* NiagaraActor = World->SpawnActor<ANiagaraActor>(
		ANiagaraActor::StaticClass(),
		Location,
		Rotation,
		SpawnParams
	);

	if (!IsValid(NiagaraActor))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn NiagaraActor"));
	}

	// Configure the Niagara component
	UNiagaraComponent* NiagaraComp = NiagaraActor->GetNiagaraComponent();
	if (!IsValid(NiagaraComp))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("NiagaraActor has no valid NiagaraComponent"));
	}

	NiagaraComp->SetAsset(NiagaraSystem);
	NiagaraComp->SetAutoActivate(bAutoActivate);

	if (bAutoActivate)
	{
		NiagaraComp->Activate(true);
	}

	// Set scale
	NiagaraActor->SetActorScale3D(Scale);

	// Mark for persistence (OFPA)
	NiagaraActor->SetFlags(RF_Transactional);
	if (NiagaraActor->GetRootComponent())
	{
		NiagaraActor->GetRootComponent()->SetFlags(RF_Transactional);
	}

	if (UPackage* ActorPackage = NiagaraActor->GetExternalPackage())
	{
		ActorPackage->SetDirtyFlag(true);
	}

	UE_LOG(LogTemp, Log, TEXT("Spawned NiagaraActor '%s' at (%f, %f, %f) with system '%s'"),
		*NiagaraActor->GetName(), Location.X, Location.Y, Location.Z, *SystemPath);

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("actor_name"), NiagaraActor->GetName());
	ResultObj->SetStringField(TEXT("system_path"), SystemPath);
	ResultObj->SetBoolField(TEXT("auto_activate"), bAutoActivate);

	TArray<TSharedPtr<FJsonValue>> LocationArray;
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
	LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
	ResultObj->SetArrayField(TEXT("location"), LocationArray);

	TArray<TSharedPtr<FJsonValue>> ScaleArray;
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
	ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
	ResultObj->SetArrayField(TEXT("scale"), ScaleArray);

	return ResultObj;
}

// ============================================================================
// 7. HandleSetSkeletalAnimation
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleSetSkeletalAnimation(const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName = Params->GetStringField(TEXT("actor_name"));
	FString AnimationPath = Params->GetStringField(TEXT("animation_path"));
	bool bLooping = true;
	if (Params->HasField(TEXT("looping")))
	{
		bLooping = Params->GetBoolField(TEXT("looping"));
	}
	float PlayRate = 1.0f;
	if (Params->HasField(TEXT("play_rate")))
	{
		PlayRate = Params->GetNumberField(TEXT("play_rate"));
	}
	FString ComponentName = TEXT("");
	if (Params->HasField(TEXT("component_name")))
	{
		ComponentName = Params->GetStringField(TEXT("component_name"));
	}

	if (ActorName.IsEmpty() || AnimationPath.IsEmpty())
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("actor_name and animation_path are required"));
	}

	// Find the actor
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world found"));
	}

	AActor* TargetActor = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->GetName() == ActorName || It->GetActorLabel() == ActorName)
		{
			TargetActor = *It;
			break;
		}
	}

	if (!TargetActor)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
	}

	// Find the SkeletalMeshComponent
	USkeletalMeshComponent* SkelMeshComp = nullptr;

	if (!ComponentName.IsEmpty())
	{
		// Find by name
		for (UActorComponent* Comp : TargetActor->GetComponents())
		{
			if (Comp && Comp->GetName() == ComponentName)
			{
				SkelMeshComp = Cast<USkeletalMeshComponent>(Comp);
				break;
			}
		}
	}
	else
	{
		// Try ACharacter first (CDO mesh = CharacterMesh0)
		ACharacter* Character = Cast<ACharacter>(TargetActor);
		if (Character)
		{
			SkelMeshComp = Character->GetMesh();
		}
		else
		{
			// Find first SkeletalMeshComponent on the actor
			SkelMeshComp = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
		}
	}

	if (!SkelMeshComp)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No SkeletalMeshComponent found on actor"));
	}

	// Load the animation asset
	UAnimSequenceBase* AnimSequence = Cast<UAnimSequenceBase>(
		StaticLoadObject(UAnimSequenceBase::StaticClass(), nullptr, *AnimationPath));

	if (!AnimSequence)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Animation not found: %s"), *AnimationPath));
	}

	// Clear any AnimBP class so it doesn't override SingleNode mode in PIE
	SkelMeshComp->SetAnimInstanceClass(nullptr);

	// Also clear AnimBP on the Blueprint CDO so PIE doesn't re-apply it
	// (PIE reconstructs actors from their Blueprint class — CDO AnimClass overrides per-instance settings)
	UBlueprint* OwnerBP = nullptr;
	if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(TargetActor->GetClass()))
	{
		OwnerBP = Cast<UBlueprint>(BPGC->ClassGeneratedBy);
		if (OwnerBP && OwnerBP->GeneratedClass)
		{
			ACharacter* CDO = Cast<ACharacter>(OwnerBP->GeneratedClass->GetDefaultObject());
			if (CDO)
			{
				USkeletalMeshComponent* CDOMesh = CDO->GetMesh();
				if (CDOMesh)
				{
					CDOMesh->SetAnimInstanceClass(nullptr);
					CDOMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				}
			}
		}
	}

	// Switch to AnimationSingleNode mode and set the animation
	SkelMeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SkelMeshComp->OverrideAnimationData(AnimSequence, bLooping, /*bIsPlaying=*/true, /*Position=*/0.0f, PlayRate);

	// Enable animation in editor for preview (transient, resets on load)
	SkelMeshComp->SetUpdateAnimationInEditor(true);

	// Mark dirty for save
	TargetActor->Modify();
	TargetActor->MarkPackageDirty();

	// Also mark the Blueprint package dirty if we modified the CDO
	if (OwnerBP)
	{
		OwnerBP->GetPackage()->MarkPackageDirty();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(OwnerBP);
		FKismetEditorUtilities::CompileBlueprint(OwnerBP);
	}

	// Build result
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("actor"), ActorName);
	ResultObj->SetStringField(TEXT("animation"), AnimationPath);
	ResultObj->SetBoolField(TEXT("looping"), bLooping);
	ResultObj->SetNumberField(TEXT("play_rate"), PlayRate);
	ResultObj->SetStringField(TEXT("animation_mode"), TEXT("AnimationSingleNode"));
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Animation '%s' set on actor '%s' (looping=%s, rate=%.2f)"),
		*AnimationPath, *ActorName, bLooping ? TEXT("true") : TEXT("false"), PlayRate));

	return ResultObj;
}

// ============================================================================
// 8. HandleCreateNiagaraSystem
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleCreateNiagaraSystem(const TSharedPtr<FJsonObject>& Params)
{
	// Required: system_name
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_name' parameter"));
	}

	// Optional: destination_path (default /Game/FX)
	FString DestinationPath = TEXT("/Game/FX");
	Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

	// Optional: template_emitter_path (default HangingParticulates)
	FString TemplatePath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/HangingParticulates");
	Params->TryGetStringField(TEXT("template_emitter_path"), TemplatePath);

	// Build full asset path
	FString FullAssetPath = DestinationPath / SystemName;

	// Load template emitter
	UNiagaraEmitter* TemplateEmitter = Cast<UNiagaraEmitter>(
		StaticLoadObject(UNiagaraEmitter::StaticClass(), nullptr, *TemplatePath));

	if (!IsValid(TemplateEmitter))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Failed to load template emitter at path: %s"), *TemplatePath));
	}

	// Create package
	UPackage* Package = CreatePackage(*FullAssetPath);
	if (!Package)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Failed to create package at: %s"), *FullAssetPath));
	}

	// Create the Niagara system
	UNiagaraSystem* NewSystem = NewObject<UNiagaraSystem>(
		Package, FName(*SystemName), RF_Public | RF_Standalone);

	if (!NewSystem)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UNiagaraSystem object"));
	}

	// Initialize the system
	UNiagaraSystemFactoryNew::InitializeSystem(NewSystem, true /* bCanBeInCluster */);

	// Add emitter to system (UE 5.7 API: emitter + version GUID as separate params)
	FNiagaraEditorUtilities::AddEmitterToSystem(*NewSystem, *TemplateEmitter, TemplateEmitter->GetExposedVersion().VersionGuid, true /* bCopy */);

	// Request compile
	NewSystem->RequestCompile(false);

	// Wait for compilation to complete
	NewSystem->WaitForCompilationComplete();

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(NewSystem);

	// Mark package dirty and save
	Package->MarkPackageDirty();

	FString PackageFilename = FPackageName::LongPackageNameToFilename(
		FullAssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewSystem, *PackageFilename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Created NiagaraSystem '%s' at '%s' from template '%s'"),
		*SystemName, *FullAssetPath, *TemplatePath);

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("system_path"), FullAssetPath);
	ResultObj->SetStringField(TEXT("system_name"), SystemName);
	ResultObj->SetStringField(TEXT("template_used"), TemplatePath);

	return ResultObj;
}

// ============================================================================
// 9. HandleSetNiagaraParameter
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleSetNiagaraParameter(const TSharedPtr<FJsonObject>& Params)
{
	// Required: actor_name
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	// Required: parameter_name
	FString ParamName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParamName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_name' parameter"));
	}

	// Required: parameter_type
	FString ParamType;
	if (!Params->TryGetStringField(TEXT("parameter_type"), ParamType))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'parameter_type' parameter. Must be one of: float, int, bool, vector, vector2d, position, color"));
	}

	// Required: value
	if (!Params->HasField(TEXT("value")))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	// Get world
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
	}

	// Find actor
	AActor* FoundActor = FEpicUnrealMCPCommonUtils::FindActorByName(World, ActorName);
	if (!IsValid(FoundActor))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Actor not found: %s"), *ActorName));
	}

	// Cast to NiagaraActor
	ANiagaraActor* NiagaraActor = Cast<ANiagaraActor>(FoundActor);
	if (!NiagaraActor)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Actor '%s' is not a NiagaraActor"), *ActorName));
	}

	// Get Niagara component
	UNiagaraComponent* NiagaraComp = NiagaraActor->GetNiagaraComponent();
	if (!IsValid(NiagaraComp))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("NiagaraActor has no valid NiagaraComponent"));
	}

	// Parse value based on parameter_type
	FString ValueSet;

	if (ParamType == TEXT("float"))
	{
		double Value = Params->GetNumberField(TEXT("value"));
		NiagaraComp->SetVariableFloat(FName(*ParamName), (float)Value);
		ValueSet = FString::Printf(TEXT("%.4f"), Value);
	}
	else if (ParamType == TEXT("int"))
	{
		int32 Value = (int32)Params->GetNumberField(TEXT("value"));
		NiagaraComp->SetVariableInt(FName(*ParamName), Value);
		ValueSet = FString::Printf(TEXT("%d"), Value);
	}
	else if (ParamType == TEXT("bool"))
	{
		bool bValue = Params->GetBoolField(TEXT("value"));
		NiagaraComp->SetVariableBool(FName(*ParamName), bValue);
		ValueSet = bValue ? TEXT("true") : TEXT("false");
	}
	else if (ParamType == TEXT("vector") || ParamType == TEXT("position"))
	{
		FVector Vec = FEpicUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("value"));
		if (ParamType == TEXT("vector"))
		{
			NiagaraComp->SetVariableVec3(FName(*ParamName), Vec);
		}
		else
		{
			NiagaraComp->SetVariablePosition(FName(*ParamName), Vec);
		}
		ValueSet = FString::Printf(TEXT("(%.2f, %.2f, %.2f)"), Vec.X, Vec.Y, Vec.Z);
	}
	else if (ParamType == TEXT("vector2d"))
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray = nullptr;
		if (Params->TryGetArrayField(TEXT("value"), ValueArray) && ValueArray && ValueArray->Num() >= 2)
		{
			float X = (float)(*ValueArray)[0]->AsNumber();
			float Y = (float)(*ValueArray)[1]->AsNumber();
			NiagaraComp->SetVariableVec2(FName(*ParamName), FVector2D(X, Y));
			ValueSet = FString::Printf(TEXT("(%.2f, %.2f)"), X, Y);
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid vector2d value. Expected array [X, Y]"));
		}
	}
	else if (ParamType == TEXT("color"))
	{
		const TSharedPtr<FJsonObject>* ColorObj = nullptr;
		if (Params->TryGetObjectField(TEXT("value"), ColorObj) && ColorObj && (*ColorObj).IsValid())
		{
			float R = (float)(*ColorObj)->GetNumberField(TEXT("R"));
			float G = (float)(*ColorObj)->GetNumberField(TEXT("G"));
			float B = (float)(*ColorObj)->GetNumberField(TEXT("B"));
			float A = (*ColorObj)->HasField(TEXT("A")) ? (float)(*ColorObj)->GetNumberField(TEXT("A")) : 1.0f;
			NiagaraComp->SetVariableLinearColor(FName(*ParamName), FLinearColor(R, G, B, A));
			ValueSet = FString::Printf(TEXT("(R=%.2f, G=%.2f, B=%.2f, A=%.2f)"), R, G, B, A);
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid color value. Expected object {\"R\":...,\"G\":...,\"B\":...,\"A\":...}"));
		}
	}
	else
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Invalid parameter_type: '%s'. Must be one of: float, int, bool, vector, vector2d, position, color"), *ParamType));
	}

	UE_LOG(LogTemp, Log, TEXT("Set Niagara parameter '%s' on actor '%s' to %s (type: %s)"),
		*ParamName, *ActorName, *ValueSet, *ParamType);

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("actor_name"), ActorName);
	ResultObj->SetStringField(TEXT("parameter_name"), ParamName);
	ResultObj->SetStringField(TEXT("parameter_type"), ParamType);
	ResultObj->SetStringField(TEXT("value_set"), ValueSet);

	return ResultObj;
}

// ============================================================================
// 10. HandleCreateAtmosphericFX
// ============================================================================
TSharedPtr<FJsonObject> FEpicUnrealMCPGameplayCommands::HandleCreateAtmosphericFX(const TSharedPtr<FJsonObject>& Params)
{
	// Required: system_name
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'system_name' parameter"));
	}

	// Required: preset
	FString Preset;
	if (!Params->TryGetStringField(TEXT("preset"), Preset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'preset' parameter"));
	}

	// Validate preset
	if (Preset != TEXT("sandstorm") && Preset != TEXT("ground_mist") && Preset != TEXT("floating_dust"))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Invalid preset '%s'. Must be one of: sandstorm, ground_mist, floating_dust"), *Preset));
	}

	// Optional: destination_path (default /Game/FX)
	FString DestinationPath = TEXT("/Game/FX");
	Params->TryGetStringField(TEXT("destination_path"), DestinationPath);

	// Build full asset path
	FString FullAssetPath = DestinationPath / SystemName;

	// Load Minimal template emitter
	FString TemplatePath = TEXT("/Niagara/DefaultAssets/Templates/Emitters/Minimal");
	UNiagaraEmitter* TemplateEmitter = Cast<UNiagaraEmitter>(
		StaticLoadObject(UNiagaraEmitter::StaticClass(), nullptr, *TemplatePath));

	if (!IsValid(TemplateEmitter))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Failed to load Minimal template emitter at path: %s"), *TemplatePath));
	}

	// Create package
	UPackage* Package = CreatePackage(*FullAssetPath);
	if (!Package)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
			TEXT("Failed to create package at: %s"), *FullAssetPath));
	}

	// Create the Niagara system
	UNiagaraSystem* NewSystem = NewObject<UNiagaraSystem>(
		Package, FName(*SystemName), RF_Public | RF_Standalone);

	if (!NewSystem)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UNiagaraSystem object"));
	}

	// Initialize the system
	UNiagaraSystemFactoryNew::InitializeSystem(NewSystem, true /* bCanBeInCluster */);

	// Add emitter to system (UE 5.7 API: emitter + version GUID as separate params)
	FNiagaraEditorUtilities::AddEmitterToSystem(*NewSystem, *TemplateEmitter, TemplateEmitter->GetExposedVersion().VersionGuid, true /* bCopy */);

	// Get emitter handle from system
	TArray<FNiagaraEmitterHandle>& EmitterHandles = NewSystem->GetEmitterHandles();
	if (EmitterHandles.Num() == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No emitter handles found in system after adding emitter"));
	}

	// Get the emitter we just added
	FVersionedNiagaraEmitter VersionedEmitter = EmitterHandles[0].GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get emitter from handle"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	UNiagaraScriptSource* ScriptSource = Cast<UNiagaraScriptSource>(EmitterData->GraphSource);
	if (!ScriptSource)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get script source from emitter"));
	}

	UNiagaraGraph* Graph = ScriptSource->NodeGraph;
	if (!Graph)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get node graph from script source"));
	}

	// Mark graph as modified
	Graph->Modify();

	// Find output nodes for each script stage (using FindEquivalentOutputNode which is exported)
	UNiagaraNodeOutput* EmitterUpdateOutput = Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::EmitterUpdateScript);
	UNiagaraNodeOutput* ParticleSpawnOutput = Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleSpawnScript);
	UNiagaraNodeOutput* ParticleUpdateOutput = Graph->FindEquivalentOutputNode(ENiagaraScriptUsage::ParticleUpdateScript);

	if (!EmitterUpdateOutput)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find EmitterUpdateScript output node"));
	}
	if (!ParticleSpawnOutput)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find ParticleSpawnScript output node"));
	}
	if (!ParticleUpdateOutput)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to find ParticleUpdateScript output node"));
	}

	// Helper lambda to load and add a module script
	TArray<FString> ModulesAdded;
	TArray<FString> ModulesFailed;

	auto AddModule = [&](const TCHAR* ModulePath, UNiagaraNodeOutput* OutputNode, const FString& Name) -> bool
	{
		UNiagaraScript* Script = Cast<UNiagaraScript>(
			StaticLoadObject(UNiagaraScript::StaticClass(), nullptr, ModulePath));
		if (!Script)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load Niagara module: %s"), ModulePath);
			ModulesFailed.Add(Name);
			return false;
		}

		UNiagaraNodeFunctionCall* Node = FNiagaraStackGraphUtilities::AddScriptModuleToStack(
			Script, *OutputNode, INDEX_NONE, Name);

		if (!Node)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to add module to stack: %s"), *Name);
			ModulesFailed.Add(Name);
			return false;
		}

		UE_LOG(LogTemp, Log, TEXT("Successfully added module: %s"), *Name);
		ModulesAdded.Add(Name);
		return true;
	};

	// Add modules based on preset
	if (Preset == TEXT("sandstorm"))
	{
		// EmitterUpdate: SpawnRate
		AddModule(TEXT("/Niagara/Modules/Emitter/SpawnRate"), EmitterUpdateOutput, TEXT("SpawnRate"));

		// ParticleSpawn: InitializeParticle, BoxLocation, AddVelocity
		AddModule(TEXT("/Niagara/Modules/Spawn/Initialization/InitializeParticle"), ParticleSpawnOutput, TEXT("InitializeParticle"));
		AddModule(TEXT("/Niagara/Modules/Spawn/Location/BoxLocation"), ParticleSpawnOutput, TEXT("BoxLocation"));
		AddModule(TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocity"), ParticleSpawnOutput, TEXT("AddVelocity"));

		// ParticleUpdate: CurlNoiseForce, Drag, GravityForce, SolveForcesAndVelocity
		AddModule(TEXT("/Niagara/Modules/Update/Forces/CurlNoiseForce"), ParticleUpdateOutput, TEXT("CurlNoiseForce"));
		AddModule(TEXT("/Niagara/Modules/Update/Forces/Drag"), ParticleUpdateOutput, TEXT("Drag"));
		AddModule(TEXT("/Niagara/Modules/Update/Forces/GravityForce"), ParticleUpdateOutput, TEXT("GravityForce"));
		AddModule(TEXT("/Niagara/Modules/Solvers/SolveForcesAndVelocity"), ParticleUpdateOutput, TEXT("SolveForcesAndVelocity"));
	}
	else if (Preset == TEXT("ground_mist"))
	{
		// EmitterUpdate: SpawnRate
		AddModule(TEXT("/Niagara/Modules/Emitter/SpawnRate"), EmitterUpdateOutput, TEXT("SpawnRate"));

		// ParticleSpawn: InitializeParticle, BoxLocation, AddVelocity
		AddModule(TEXT("/Niagara/Modules/Spawn/Initialization/InitializeParticle"), ParticleSpawnOutput, TEXT("InitializeParticle"));
		AddModule(TEXT("/Niagara/Modules/Spawn/Location/BoxLocation"), ParticleSpawnOutput, TEXT("BoxLocation"));
		AddModule(TEXT("/Niagara/Modules/Spawn/Velocity/AddVelocity"), ParticleSpawnOutput, TEXT("AddVelocity"));

		// ParticleUpdate: CurlNoiseForce, Drag, SolveForcesAndVelocity
		AddModule(TEXT("/Niagara/Modules/Update/Forces/CurlNoiseForce"), ParticleUpdateOutput, TEXT("CurlNoiseForce"));
		AddModule(TEXT("/Niagara/Modules/Update/Forces/Drag"), ParticleUpdateOutput, TEXT("Drag"));
		AddModule(TEXT("/Niagara/Modules/Solvers/SolveForcesAndVelocity"), ParticleUpdateOutput, TEXT("SolveForcesAndVelocity"));
	}
	else if (Preset == TEXT("floating_dust"))
	{
		// EmitterUpdate: SpawnRate
		AddModule(TEXT("/Niagara/Modules/Emitter/SpawnRate"), EmitterUpdateOutput, TEXT("SpawnRate"));

		// ParticleSpawn: InitializeParticle, BoxLocation
		AddModule(TEXT("/Niagara/Modules/Spawn/Initialization/InitializeParticle"), ParticleSpawnOutput, TEXT("InitializeParticle"));
		AddModule(TEXT("/Niagara/Modules/Spawn/Location/BoxLocation"), ParticleSpawnOutput, TEXT("BoxLocation"));

		// ParticleUpdate: CurlNoiseForce, GravityForce, SolveForcesAndVelocity
		AddModule(TEXT("/Niagara/Modules/Update/Forces/CurlNoiseForce"), ParticleUpdateOutput, TEXT("CurlNoiseForce"));
		AddModule(TEXT("/Niagara/Modules/Update/Forces/GravityForce"), ParticleUpdateOutput, TEXT("GravityForce"));
		AddModule(TEXT("/Niagara/Modules/Solvers/SolveForcesAndVelocity"), ParticleUpdateOutput, TEXT("SolveForcesAndVelocity"));
	}

	// Notify graph changed
	Graph->NotifyGraphChanged();

	// Request compile
	NewSystem->RequestCompile(false);

	// Wait for compilation to complete
	NewSystem->WaitForCompilationComplete();

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(NewSystem);

	// Mark package dirty and save
	Package->MarkPackageDirty();

	FString PackageFilename = FPackageName::LongPackageNameToFilename(
		FullAssetPath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewSystem, *PackageFilename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("Created atmospheric FX system '%s' at '%s' with preset '%s'"),
		*SystemName, *FullAssetPath, *Preset);

	// Build response
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("system_path"), FullAssetPath);
	ResultObj->SetStringField(TEXT("system_name"), SystemName);
	ResultObj->SetStringField(TEXT("preset"), Preset);

	// Add modules arrays
	TArray<TSharedPtr<FJsonValue>> AddedArray;
	for (const FString& ModuleName : ModulesAdded)
	{
		AddedArray.Add(MakeShared<FJsonValueString>(ModuleName));
	}
	ResultObj->SetArrayField(TEXT("modules_added"), AddedArray);

	TArray<TSharedPtr<FJsonValue>> FailedArray;
	for (const FString& ModuleName : ModulesFailed)
	{
		FailedArray.Add(MakeShared<FJsonValueString>(ModuleName));
	}
	ResultObj->SetArrayField(TEXT("modules_failed"), FailedArray);

	return ResultObj;
}
