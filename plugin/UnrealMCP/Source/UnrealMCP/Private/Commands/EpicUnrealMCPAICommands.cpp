#include "Commands/EpicUnrealMCPAICommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Kismet/GameplayStatics.h"

// Behavior Tree
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTDecorator.h"

// BT Tasks
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "BehaviorTree/Tasks/BTTask_RunEQSQuery.h"

// BT Decorators
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Decorators/BTDecorator_Cooldown.h"
#include "BehaviorTree/Decorators/BTDecorator_TimeLimit.h"
#include "BehaviorTree/Decorators/BTDecorator_IsAtLocation.h"

// Blackboard
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/BlackboardComponent.h"

// AI Controller
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

// BT Composites
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"

// ---------------------------------------------------------------------------

FEpicUnrealMCPAICommands::FEpicUnrealMCPAICommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPAICommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_behavior_tree"))
	{
		return HandleCreateBehaviorTree(Params);
	}
	else if (CommandType == TEXT("create_blackboard"))
	{
		return HandleCreateBlackboard(Params);
	}
	else if (CommandType == TEXT("add_bt_task"))
	{
		return HandleAddBTTask(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator"))
	{
		return HandleAddBTDecorator(Params);
	}
	else if (CommandType == TEXT("assign_behavior_tree"))
	{
		return HandleAssignBehaviorTree(Params);
	}

	return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown AI command: %s"), *CommandType));
}

// ---------------------------------------------------------------------------
// Helper: save a package to disk (shared by multiple handlers)
// ---------------------------------------------------------------------------
static bool SaveAssetPackage(UPackage* Package, UObject* Asset, const FString& FullPackagePath)
{
	if (!Package || !Asset)
	{
		return false;
	}

	FString PackageFilename = FPackageName::LongPackageNameToFilename(
		FullPackagePath, FPackageName::GetAssetPackageExtension());

	FString PackageDirectory = FPaths::GetPath(PackageFilename);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*PackageDirectory))
	{
		PlatformFile.CreateDirectoryTree(*PackageDirectory);
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Package, Asset, *PackageFilename, SaveArgs);
}

// ---------------------------------------------------------------------------
// create_behavior_tree
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPAICommands::HandleCreateBehaviorTree(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: bt_name ---
	FString BTName;
	if (!Params->TryGetStringField(TEXT("bt_name"), BTName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'bt_name' parameter"));
	}

	// --- Optional: bt_path (default /Game/AI/) ---
	FString BTPath;
	if (!Params->TryGetStringField(TEXT("bt_path"), BTPath))
	{
		BTPath = TEXT("/Game/AI");
	}
	if (BTPath.EndsWith(TEXT("/")))
	{
		BTPath.LeftChopInline(1);
	}

	// --- Optional: root_type (Selector or Sequence, default Selector) ---
	FString RootType;
	if (!Params->TryGetStringField(TEXT("root_type"), RootType))
	{
		RootType = TEXT("Selector");
	}

	FString FullPackagePath = BTPath / BTName;

	// --- Check if asset already exists ---
	if (UEditorAssetLibrary::DoesAssetExist(FullPackagePath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Behavior tree already exists at: %s"), *FullPackagePath));
	}

	// --- Create package ---
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create package: %s"), *FullPackagePath));
	}
	Package->FullyLoad();

	// --- Create behavior tree asset ---
	UBehaviorTree* BT = NewObject<UBehaviorTree>(Package, FName(*BTName), RF_Public | RF_Standalone);
	if (!BT)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UBehaviorTree object"));
	}

	// --- Create root composite node ---
	UBTCompositeNode* RootNode = nullptr;
	if (RootType == TEXT("Sequence"))
	{
		RootNode = NewObject<UBTComposite_Sequence>(BT);
	}
	else
	{
		RootNode = NewObject<UBTComposite_Selector>(BT);
		RootType = TEXT("Selector"); // normalize for response
	}

	if (!RootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create root composite node"));
	}

	BT->RootNode = RootNode;

	// --- Register and save ---
	FAssetRegistryModule::AssetCreated(BT);
	Package->MarkPackageDirty();

	bool bSaved = SaveAssetPackage(Package, BT, FullPackagePath);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("create_behavior_tree: Failed to save package to disk: %s"), *FullPackagePath);
	}

	// --- Build response ---
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("bt_name"), BTName);
	Data->SetStringField(TEXT("full_path"), FullPackagePath);
	Data->SetStringField(TEXT("root_type"), RootType);
	Data->SetBoolField(TEXT("saved"), bSaved);

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// create_blackboard
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPAICommands::HandleCreateBlackboard(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: bb_name ---
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'bb_name' parameter"));
	}

	// --- Optional: bb_path (default /Game/AI/) ---
	FString BBPath;
	if (!Params->TryGetStringField(TEXT("bb_path"), BBPath))
	{
		BBPath = TEXT("/Game/AI");
	}
	if (BBPath.EndsWith(TEXT("/")))
	{
		BBPath.LeftChopInline(1);
	}

	// --- Required: keys array ---
	const TArray<TSharedPtr<FJsonValue>>* KeysArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("keys"), KeysArray) || !KeysArray || KeysArray->Num() == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or empty 'keys' array parameter"));
	}

	FString FullPackagePath = BBPath / BBName;

	// --- Check if asset already exists ---
	if (UEditorAssetLibrary::DoesAssetExist(FullPackagePath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blackboard already exists at: %s"), *FullPackagePath));
	}

	// --- Create package ---
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create package: %s"), *FullPackagePath));
	}
	Package->FullyLoad();

	// --- Create blackboard asset ---
	UBlackboardData* BB = NewObject<UBlackboardData>(Package, FName(*BBName), RF_Public | RF_Standalone);
	if (!BB)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create UBlackboardData object"));
	}

	// --- Populate keys ---
	int32 KeyCount = 0;
	TArray<TSharedPtr<FJsonValue>> FailedKeys;

	for (const TSharedPtr<FJsonValue>& KeyValue : *KeysArray)
	{
		const TSharedPtr<FJsonObject>* KeyObjPtr = nullptr;
		if (!KeyValue->TryGetObject(KeyObjPtr) || !KeyObjPtr || !(*KeyObjPtr).IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("create_blackboard: Skipping invalid key entry"));
			continue;
		}
		const TSharedPtr<FJsonObject>& KeyObj = *KeyObjPtr;

		FString KeyName;
		if (!KeyObj->TryGetStringField(TEXT("name"), KeyName))
		{
			UE_LOG(LogTemp, Warning, TEXT("create_blackboard: Key missing 'name', skipping"));
			continue;
		}

		FString KeyType;
		if (!KeyObj->TryGetStringField(TEXT("type"), KeyType))
		{
			UE_LOG(LogTemp, Warning, TEXT("create_blackboard: Key '%s' missing 'type', skipping"), *KeyName);
			continue;
		}

		// --- Create the key type instance ---
		UBlackboardKeyType* KeyTypeInstance = nullptr;

		if (KeyType == TEXT("Object"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Object>(BB);
		}
		else if (KeyType == TEXT("Bool"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Bool>(BB);
		}
		else if (KeyType == TEXT("Int"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Int>(BB);
		}
		else if (KeyType == TEXT("Float"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Float>(BB);
		}
		else if (KeyType == TEXT("Vector"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Vector>(BB);
		}
		else if (KeyType == TEXT("String"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_String>(BB);
		}
		else if (KeyType == TEXT("Enum"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Enum>(BB);
		}
		else if (KeyType == TEXT("Class"))
		{
			KeyTypeInstance = NewObject<UBlackboardKeyType_Class>(BB);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("create_blackboard: Unknown key type '%s' for key '%s', skipping"), *KeyType, *KeyName);
			continue;
		}

		if (!KeyTypeInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("create_blackboard: Failed to create key type instance for '%s'"), *KeyName);
			continue;
		}

		FBlackboardEntry Entry;
		Entry.EntryName = FName(*KeyName);
		Entry.KeyType = KeyTypeInstance;
		BB->Keys.Add(Entry);
		KeyCount++;
	}

	if (KeyCount == 0)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No valid keys were added to blackboard"));
	}

	// --- Register and save ---
	FAssetRegistryModule::AssetCreated(BB);
	Package->MarkPackageDirty();

	bool bSaved = SaveAssetPackage(Package, BB, FullPackagePath);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("create_blackboard: Failed to save package to disk: %s"), *FullPackagePath);
	}

	// --- Build response ---
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("bb_name"), BBName);
	Data->SetStringField(TEXT("full_path"), FullPackagePath);
	Data->SetNumberField(TEXT("key_count"), KeyCount);
	Data->SetBoolField(TEXT("saved"), bSaved);

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// add_bt_task
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPAICommands::HandleAddBTTask(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: bt_path ---
	FString BTPath;
	if (!Params->TryGetStringField(TEXT("bt_path"), BTPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'bt_path' parameter"));
	}

	// --- Required: task_type ---
	FString TaskType;
	if (!Params->TryGetStringField(TEXT("task_type"), TaskType))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'task_type' parameter"));
	}

	// --- Optional: task_params ---
	const TSharedPtr<FJsonObject>* TaskParamsPtr = nullptr;
	TSharedPtr<FJsonObject> TaskParams;
	if (Params->TryGetObjectField(TEXT("task_params"), TaskParamsPtr) && TaskParamsPtr)
	{
		TaskParams = *TaskParamsPtr;
	}

	// --- Load behavior tree ---
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(BTPath);
	if (!IsValid(LoadedAsset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load behavior tree at: %s"), *BTPath));
	}

	UBehaviorTree* BT = Cast<UBehaviorTree>(LoadedAsset);
	if (!BT)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not a BehaviorTree: %s"), *BTPath));
	}

	// --- Get root composite ---
	UBTCompositeNode* RootNode = Cast<UBTCompositeNode>(BT->RootNode);
	if (!RootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Behavior tree has no root composite node"));
	}

	// --- Create task based on type ---
	UBTTaskNode* NewTask = nullptr;

	if (TaskType == TEXT("MoveTo"))
	{
		UBTTask_MoveTo* MoveToTask = NewObject<UBTTask_MoveTo>(BT);
		if (MoveToTask && TaskParams.IsValid())
		{
			double AcceptableRadius = 0.0;
			if (TaskParams->TryGetNumberField(TEXT("acceptable_radius"), AcceptableRadius))
			{
				MoveToTask->AcceptableRadius = static_cast<float>(AcceptableRadius);
			}

			// Optional: blackboard key for target
			// NOTE: BlackboardKey is protected in UE5.7 - must be configured in BT editor
			FString BBKeyName;
			if (TaskParams->TryGetStringField(TEXT("blackboard_key"), BBKeyName))
			{
				UE_LOG(LogTemp, Warning, TEXT("add_bt_task: BlackboardKey assignment is not supported via MCP. Configure '%s' in the BT editor."), *BBKeyName);
			}
		}
		NewTask = MoveToTask;
	}
	else if (TaskType == TEXT("Wait"))
	{
		UBTTask_Wait* WaitTask = NewObject<UBTTask_Wait>(BT);
		if (WaitTask && TaskParams.IsValid())
		{
			double WaitTime = 5.0;
			if (TaskParams->TryGetNumberField(TEXT("wait_time"), WaitTime))
			{
				WaitTask->WaitTime = static_cast<float>(WaitTime);
			}

			double RandomDeviation = 0.0;
			if (TaskParams->TryGetNumberField(TEXT("random_deviation"), RandomDeviation))
			{
				WaitTask->RandomDeviation = static_cast<float>(RandomDeviation);
			}
		}
		NewTask = WaitTask;
	}
	else if (TaskType == TEXT("PlayAnimation"))
	{
		UBTTask_PlayAnimation* AnimTask = NewObject<UBTTask_PlayAnimation>(BT);
		if (AnimTask && TaskParams.IsValid())
		{
			FString AnimPath;
			if (TaskParams->TryGetStringField(TEXT("animation_path"), AnimPath))
			{
				UAnimationAsset* Anim = Cast<UAnimationAsset>(UEditorAssetLibrary::LoadAsset(AnimPath));
				if (Anim)
				{
					// NOTE: In UE5.7, AnimationToPlay is FValueOrBBKey_Object - must configure in BT editor
					UE_LOG(LogTemp, Warning, TEXT("add_bt_task: AnimationToPlay assignment is not supported via MCP. Configure '%s' in the BT editor."), *AnimPath);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("add_bt_task: Could not load animation at: %s"), *AnimPath);
				}
			}
		}
		NewTask = AnimTask;
	}
	else if (TaskType == TEXT("RunEQSQuery"))
	{
		UBTTask_RunEQSQuery* EQSTask = NewObject<UBTTask_RunEQSQuery>(BT);
		NewTask = EQSTask;
	}
	else
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown task type: %s. Supported: MoveTo, Wait, PlayAnimation, RunEQSQuery"), *TaskType));
	}

	if (!NewTask)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create task of type: %s"), *TaskType));
	}

	// --- Add task as child of root composite ---
	FBTCompositeChild ChildEntry;
	ChildEntry.ChildTask = NewTask;
	RootNode->Children.Add(ChildEntry);

	int32 ChildIndex = RootNode->Children.Num() - 1;

	// --- Mark dirty ---
	BT->GetPackage()->MarkPackageDirty();

	// --- Build response ---
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("bt_path"), BTPath);
	Data->SetStringField(TEXT("task_type"), TaskType);
	Data->SetNumberField(TEXT("child_index"), ChildIndex);
	Data->SetNumberField(TEXT("total_children"), RootNode->Children.Num());

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// add_bt_decorator
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPAICommands::HandleAddBTDecorator(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: bt_path ---
	FString BTPath;
	if (!Params->TryGetStringField(TEXT("bt_path"), BTPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'bt_path' parameter"));
	}

	// --- Required: decorator_type ---
	FString DecoratorType;
	if (!Params->TryGetStringField(TEXT("decorator_type"), DecoratorType))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'decorator_type' parameter"));
	}

	// --- Optional: child_index (default 0) ---
	int32 ChildIndex = 0;
	double ChildIndexDouble = 0.0;
	if (Params->TryGetNumberField(TEXT("child_index"), ChildIndexDouble))
	{
		ChildIndex = static_cast<int32>(ChildIndexDouble);
	}

	// --- Optional: decorator_params ---
	const TSharedPtr<FJsonObject>* DecParamsPtr = nullptr;
	TSharedPtr<FJsonObject> DecParams;
	if (Params->TryGetObjectField(TEXT("decorator_params"), DecParamsPtr) && DecParamsPtr)
	{
		DecParams = *DecParamsPtr;
	}

	// --- Load behavior tree ---
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(BTPath);
	if (!IsValid(LoadedAsset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load behavior tree at: %s"), *BTPath));
	}

	UBehaviorTree* BT = Cast<UBehaviorTree>(LoadedAsset);
	if (!BT)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not a BehaviorTree: %s"), *BTPath));
	}

	// --- Get root composite ---
	UBTCompositeNode* RootNode = Cast<UBTCompositeNode>(BT->RootNode);
	if (!RootNode)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Behavior tree has no root composite node"));
	}

	// --- Validate child_index bounds ---
	if (ChildIndex < 0 || ChildIndex >= RootNode->Children.Num())
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("child_index %d is out of range. Root has %d children."),
				ChildIndex, RootNode->Children.Num()));
	}

	// --- Create decorator based on type ---
	UBTDecorator* NewDecorator = nullptr;

	if (DecoratorType == TEXT("Blackboard"))
	{
		UBTDecorator_Blackboard* BBDec = NewObject<UBTDecorator_Blackboard>(BT);
		if (BBDec && DecParams.IsValid())
		{
			// NOTE: BlackboardKey is protected in UE5.7 - must be configured in BT editor
			FString BBKeyName;
			if (DecParams->TryGetStringField(TEXT("blackboard_key"), BBKeyName))
			{
				UE_LOG(LogTemp, Warning, TEXT("add_bt_decorator: BlackboardKey assignment is not supported via MCP. Configure '%s' in the BT editor."), *BBKeyName);
			}
		}
		NewDecorator = BBDec;
	}
	else if (DecoratorType == TEXT("Cooldown"))
	{
		UBTDecorator_Cooldown* CDDec = NewObject<UBTDecorator_Cooldown>(BT);
		if (CDDec && DecParams.IsValid())
		{
			double CooldownTime = 5.0;
			if (DecParams->TryGetNumberField(TEXT("cooldown_time"), CooldownTime))
			{
				CDDec->CoolDownTime = static_cast<float>(CooldownTime);
			}
		}
		NewDecorator = CDDec;
	}
	else if (DecoratorType == TEXT("TimeLimit"))
	{
		UBTDecorator_TimeLimit* TLDec = NewObject<UBTDecorator_TimeLimit>(BT);
		if (TLDec && DecParams.IsValid())
		{
			double TimeLimit = 10.0;
			if (DecParams->TryGetNumberField(TEXT("time_limit"), TimeLimit))
			{
				TLDec->TimeLimit = static_cast<float>(TimeLimit);
			}
		}
		NewDecorator = TLDec;
	}
	else if (DecoratorType == TEXT("IsAtLocation"))
	{
		UBTDecorator_IsAtLocation* LocDec = NewObject<UBTDecorator_IsAtLocation>(BT);
		if (LocDec && DecParams.IsValid())
		{
			double AcceptableRadius = 50.0;
			if (DecParams->TryGetNumberField(TEXT("acceptable_radius"), AcceptableRadius))
			{
				LocDec->AcceptableRadius = static_cast<float>(AcceptableRadius);
			}
		}
		NewDecorator = LocDec;
	}
	else
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Unknown decorator type: %s. Supported: Blackboard, Cooldown, TimeLimit, IsAtLocation"), *DecoratorType));
	}

	if (!NewDecorator)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create decorator of type: %s"), *DecoratorType));
	}

	// --- Attach decorator to the specified child ---
	RootNode->Children[ChildIndex].Decorators.Add(NewDecorator);

	// --- Mark dirty ---
	BT->GetPackage()->MarkPackageDirty();

	// --- Build response ---
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("bt_path"), BTPath);
	Data->SetStringField(TEXT("decorator_type"), DecoratorType);
	Data->SetNumberField(TEXT("child_index"), ChildIndex);
	Data->SetNumberField(TEXT("decorator_count"), RootNode->Children[ChildIndex].Decorators.Num());

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// assign_behavior_tree
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPAICommands::HandleAssignBehaviorTree(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: actor_name ---
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'actor_name' parameter"));
	}

	// --- Required: bt_path ---
	FString BTPath;
	if (!Params->TryGetStringField(TEXT("bt_path"), BTPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'bt_path' parameter"));
	}

	// --- Optional: bb_path ---
	FString BBPath;
	Params->TryGetStringField(TEXT("bb_path"), BBPath);

	// --- Load behavior tree ---
	UObject* BTAsset = UEditorAssetLibrary::LoadAsset(BTPath);
	if (!IsValid(BTAsset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load behavior tree at: %s"), *BTPath));
	}

	UBehaviorTree* BT = Cast<UBehaviorTree>(BTAsset);
	if (!BT)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not a BehaviorTree: %s"), *BTPath));
	}

	// --- Optional: load blackboard ---
	UBlackboardData* BB = nullptr;
	if (!BBPath.IsEmpty())
	{
		UObject* BBAsset = UEditorAssetLibrary::LoadAsset(BBPath);
		if (!IsValid(BBAsset))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to load blackboard at: %s"), *BBPath));
		}
		BB = Cast<UBlackboardData>(BBAsset);
		if (!BB)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Asset is not a BlackboardData: %s"), *BBPath));
		}
	}

	// --- Check if PIE is active ---
	bool bIsPIE = GEditor && GEditor->IsPlayingSessionInEditor();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor_name"), ActorName);
	Data->SetStringField(TEXT("bt_path"), BTPath);
	if (!BBPath.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_path"), BBPath);
	}
	Data->SetBoolField(TEXT("is_pie_active"), bIsPIE);

	if (bIsPIE)
	{
		// --- Runtime assignment in PIE ---
		UWorld* PIEWorld = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World() != nullptr)
			{
				PIEWorld = Context.World();
				break;
			}
		}

		if (!PIEWorld)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("PIE is active but no PIE world found"));
		}

		AActor* Actor = FEpicUnrealMCPCommonUtils::FindActorByName(PIEWorld, ActorName);
		if (!IsValid(Actor))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Actor '%s' not found in PIE world"), *ActorName));
		}

		APawn* Pawn = Cast<APawn>(Actor);
		if (!Pawn)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Actor '%s' is not a Pawn, cannot have AI controller"), *ActorName));
		}

		AAIController* AIC = Cast<AAIController>(Pawn->GetController());
		if (!AIC)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Actor '%s' does not have an AAIController"), *ActorName));
		}

		// Apply blackboard if provided
		if (BB)
		{
			UBlackboardComponent* BBComp = nullptr;
			AIC->UseBlackboard(BB, BBComp);
			if (!BBComp)
			{
				UE_LOG(LogTemp, Warning, TEXT("assign_behavior_tree: UseBlackboard returned null BBComp for '%s'"), *ActorName);
			}
		}

		// Run behavior tree
		bool bRunResult = AIC->RunBehaviorTree(BT);
		Data->SetBoolField(TEXT("bt_running"), bRunResult);
		Data->SetStringField(TEXT("status"), bRunResult
			? TEXT("Behavior tree assigned and running in PIE")
			: TEXT("RunBehaviorTree returned false"));
	}
	else
	{
		// --- Design-time: find actor in editor world and provide guidance ---
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
		}

		AActor* Actor = FEpicUnrealMCPCommonUtils::FindActorByName(World, ActorName);
		if (!IsValid(Actor))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Actor '%s' not found in editor world"), *ActorName));
		}

		// Check if it is a pawn with an AI controller class
		APawn* Pawn = Cast<APawn>(Actor);
		FString PawnInfo;
		if (Pawn)
		{
			UClass* AICClass = Pawn->AIControllerClass;
			PawnInfo = AICClass
				? FString::Printf(TEXT("Pawn '%s' has AIControllerClass: %s"), *ActorName, *AICClass->GetName())
				: FString::Printf(TEXT("Pawn '%s' has no AIControllerClass set"), *ActorName);
		}
		else
		{
			PawnInfo = FString::Printf(TEXT("Actor '%s' is not a Pawn"), *ActorName);
		}

		Data->SetStringField(TEXT("pawn_info"), PawnInfo);
		Data->SetStringField(TEXT("status"),
			TEXT("Design-time: To wire the BT at runtime, set the AIController's default BehaviorTree property, "
			     "or call RunBehaviorTree in BeginPlay. Use 'add_component_to_blueprint' to add a "
			     "BehaviorTreeComponent if needed."));
	}

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}
