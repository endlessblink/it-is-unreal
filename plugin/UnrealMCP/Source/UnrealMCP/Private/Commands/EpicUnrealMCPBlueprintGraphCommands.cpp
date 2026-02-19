#include "Commands/EpicUnrealMCPBlueprintGraphCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Commands/BlueprintGraph/NodeManager.h"
#include "Commands/BlueprintGraph/BPConnector.h"
#include "Commands/BlueprintGraph/BPVariables.h"
#include "Commands/BlueprintGraph/EventManager.h"
#include "Commands/BlueprintGraph/NodeDeleter.h"
#include "Commands/BlueprintGraph/NodePropertyManager.h"
#include "Commands/BlueprintGraph/Function/FunctionManager.h"
#include "Commands/BlueprintGraph/Function/FunctionIO.h"
// Enhanced Input
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "EditorAssetLibrary.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"

FEpicUnrealMCPBlueprintGraphCommands::FEpicUnrealMCPBlueprintGraphCommands()
{
}

FEpicUnrealMCPBlueprintGraphCommands::~FEpicUnrealMCPBlueprintGraphCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("add_blueprint_node"))
    {
        return HandleAddBlueprintNode(Params);
    }
    else if (CommandType == TEXT("connect_nodes"))
    {
        return HandleConnectNodes(Params);
    }
    else if (CommandType == TEXT("create_variable"))
    {
        return HandleCreateVariable(Params);
    }
    else if (CommandType == TEXT("set_blueprint_variable_properties"))
    {
        return HandleSetVariableProperties(Params);
    }
    else if (CommandType == TEXT("add_event_node"))
    {
        return HandleAddEventNode(Params);
    }
    else if (CommandType == TEXT("delete_node"))
    {
        return HandleDeleteNode(Params);
    }
    else if (CommandType == TEXT("set_node_property"))
    {
        return HandleSetNodeProperty(Params);
    }
    else if (CommandType == TEXT("create_function"))
    {
        return HandleCreateFunction(Params);
    }
    else if (CommandType == TEXT("add_function_input"))
    {
        return HandleAddFunctionInput(Params);
    }
    else if (CommandType == TEXT("add_function_output"))
    {
        return HandleAddFunctionOutput(Params);
    }
    else if (CommandType == TEXT("delete_function"))
    {
        return HandleDeleteFunction(Params);
    }
    else if (CommandType == TEXT("rename_function"))
    {
        return HandleRenameFunction(Params);
    }
    else if (CommandType == TEXT("add_enhanced_input_action_event"))
    {
        return HandleAddEnhancedInputActionEvent(Params);
    }
    else if (CommandType == TEXT("create_input_action"))
    {
        return HandleCreateInputAction(Params);
    }
    else if (CommandType == TEXT("add_input_mapping"))
    {
        return HandleAddInputMapping(Params);
    }

    return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint graph command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleAddBlueprintNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeType;
    if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleAddBlueprintNode: Adding %s node to blueprint '%s'"), *NodeType, *BlueprintName);

    // Use the NodeManager to add the node
    return FBlueprintNodeManager::AddNode(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleConnectNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin_name"), SourcePinName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin_name' parameter"));
    }

    FString TargetNodeId;
    if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin_name"), TargetPinName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleConnectNodes: Connecting %s.%s to %s.%s in blueprint '%s'"),
        *SourceNodeId, *SourcePinName, *TargetNodeId, *TargetPinName, *BlueprintName);

    // Use the BPConnector to connect the nodes
    return FBPConnector::ConnectNodes(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleCreateVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleCreateVariable: Creating %s variable '%s' in blueprint '%s'"),
        *VariableType, *VariableName, *BlueprintName);

    // Use the BPVariables to create the variable
    return FBPVariables::CreateVariable(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleSetVariableProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleSetVariableProperties: Modifying variable '%s' in blueprint '%s'"),
        *VariableName, *BlueprintName);

    // Use the BPVariables to set the variable properties
    return FBPVariables::SetVariableProperties(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleAddEventNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleAddEventNode: Adding event '%s' to blueprint '%s'"),
        *EventName, *BlueprintName);

    // Use the EventManager to add the event node
    return FEventManager::AddEventNode(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleDeleteNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeID;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeID))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    UE_LOG(LogTemp, Display,
        TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleDeleteNode: Deleting node '%s' from blueprint '%s'"),
        *NodeID, *BlueprintName);

    return FNodeDeleter::DeleteNode(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleSetNodeProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeID;
    if (!Params->TryGetStringField(TEXT("node_id"), NodeID))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
    }

    // Check if this is semantic mode (action parameter) or legacy mode (property_name)
    bool bHasAction = Params->HasField(TEXT("action"));

    if (bHasAction)
    {
        // Semantic mode - delegate directly to SetNodeProperty
        FString Action;
        Params->TryGetStringField(TEXT("action"), Action);
        UE_LOG(LogTemp, Display,
            TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleSetNodeProperty: Semantic mode - action '%s' on node '%s' in blueprint '%s'"),
            *Action, *NodeID, *BlueprintName);
    }
    else
    {
        // Legacy mode - require property_name
        FString PropertyName;
        if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
        }

        UE_LOG(LogTemp, Display,
            TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleSetNodeProperty: Legacy mode - Setting '%s' on node '%s' in blueprint '%s'"),
            *PropertyName, *NodeID, *BlueprintName);
    }

    return FNodePropertyManager::SetNodeProperty(Params);
}


TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleCreateFunction(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleCreateFunction: Creating function '%s' in blueprint '%s'"),
        *FunctionName, *BlueprintName);

    return FFunctionManager::CreateFunction(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleAddFunctionInput(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleAddFunctionInput: Adding input '%s' to function '%s' in blueprint '%s'"),
        *ParamName, *FunctionName, *BlueprintName);

    return FFunctionIO::AddFunctionInput(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleAddFunctionOutput(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleAddFunctionOutput: Adding output '%s' to function '%s' in blueprint '%s'"),
        *ParamName, *FunctionName, *BlueprintName);

    return FFunctionIO::AddFunctionOutput(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleDeleteFunction(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString FunctionName;
    if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleDeleteFunction: Deleting function '%s' from blueprint '%s'"),
        *FunctionName, *BlueprintName);

    return FFunctionManager::DeleteFunction(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleRenameFunction(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString OldFunctionName;
    if (!Params->TryGetStringField(TEXT("old_function_name"), OldFunctionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'old_function_name' parameter"));
    }

    FString NewFunctionName;
    if (!Params->TryGetStringField(TEXT("new_function_name"), NewFunctionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'new_function_name' parameter"));
    }

    UE_LOG(LogTemp, Display, TEXT("FEpicUnrealMCPBlueprintGraphCommands::HandleRenameFunction: Renaming function '%s' to '%s' in blueprint '%s'"),
        *OldFunctionName, *NewFunctionName, *BlueprintName);

    return FFunctionManager::RenameFunction(Params);
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleAddEnhancedInputActionEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString InputActionPath;
    if (!Params->TryGetStringField(TEXT("input_action_path"), InputActionPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'input_action_path' parameter"));
    }

    // Optional position
    double PosX = 0.0, PosY = 0.0;
    Params->TryGetNumberField(TEXT("pos_x"), PosX);
    Params->TryGetNumberField(TEXT("pos_y"), PosY);

    UE_LOG(LogTemp, Display, TEXT("HandleAddEnhancedInputActionEvent: Adding EnhancedInputAction '%s' to '%s'"),
        *InputActionPath, *BlueprintName);

    // Load the Blueprint
    FString BlueprintPath = BlueprintName;
    if (!BlueprintPath.StartsWith(TEXT("/")))
    {
        BlueprintPath = TEXT("/Game/Blueprints/") + BlueprintPath;
    }
    if (!BlueprintPath.Contains(TEXT(".")))
    {
        BlueprintPath += TEXT(".") + FPaths::GetBaseFilename(BlueprintPath);
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    if (!Blueprint)
    {
        if (UEditorAssetLibrary::DoesAssetExist(BlueprintPath))
        {
            Blueprint = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(BlueprintPath));
        }
    }
    if (!Blueprint)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Load the InputAction asset
    FString IAPath = InputActionPath;
    if (!IAPath.Contains(TEXT(".")))
    {
        IAPath += TEXT(".") + FPaths::GetBaseFilename(IAPath);
    }

    UInputAction* InputAction = LoadObject<UInputAction>(nullptr, *IAPath);
    if (!InputAction)
    {
        if (UEditorAssetLibrary::DoesAssetExist(IAPath))
        {
            InputAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(IAPath));
        }
    }
    if (!InputAction)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputAction not found: %s"), *InputActionPath));
    }

    // Get the event graph
    if (Blueprint->UbergraphPages.Num() == 0)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint has no event graph"));
    }
    UEdGraph* Graph = Blueprint->UbergraphPages[0];
    if (!Graph)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get Blueprint event graph"));
    }

    // Check for existing node with same InputAction to avoid duplicates
    for (UEdGraphNode* ExistingNode : Graph->Nodes)
    {
        UK2Node_EnhancedInputAction* ExistingIA = Cast<UK2Node_EnhancedInputAction>(ExistingNode);
        if (ExistingIA && ExistingIA->InputAction == InputAction)
        {
            UE_LOG(LogTemp, Display, TEXT("HandleAddEnhancedInputActionEvent: Reusing existing node for '%s'"),
                *InputActionPath);

            // Return the existing node info
            TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
            Response->SetBoolField(TEXT("success"), true);
            Response->SetStringField(TEXT("node_id"), ExistingIA->NodeGuid.ToString());
            Response->SetStringField(TEXT("input_action"), InputActionPath);
            Response->SetNumberField(TEXT("pos_x"), ExistingIA->NodePosX);
            Response->SetNumberField(TEXT("pos_y"), ExistingIA->NodePosY);
            Response->SetBoolField(TEXT("reused_existing"), true);

            // List output pins
            TArray<TSharedPtr<FJsonValue>> PinArray;
            for (UEdGraphPin* Pin : ExistingIA->Pins)
            {
                if (Pin->Direction == EGPD_Output)
                {
                    TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
                    PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
                    PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
                    PinArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
                }
            }
            Response->SetArrayField(TEXT("output_pins"), PinArray);
            return Response;
        }
    }

    // Create the EnhancedInputAction node
    // CRITICAL: Set InputAction BEFORE AllocateDefaultPins - it determines pin types
    UK2Node_EnhancedInputAction* ActionNode = NewObject<UK2Node_EnhancedInputAction>(Graph);
    ActionNode->InputAction = InputAction;
    ActionNode->NodePosX = static_cast<int32>(PosX);
    ActionNode->NodePosY = static_cast<int32>(PosY);

    Graph->AddNode(ActionNode, true);
    ActionNode->PostPlacedNewNode();
    ActionNode->AllocateDefaultPins();

    // Auto-split the ActionValue pin for Axis2D/3D actions so sub-pins are immediately usable
    UEdGraphPin* ActionValuePin = ActionNode->FindPin(TEXT("ActionValue"), EGPD_Output);
    if (ActionValuePin && ActionValuePin->SubPins.Num() == 0 &&
        ActionValuePin->PinType.PinCategory != UEdGraphSchema_K2::PC_Boolean)
    {
        const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
        if (K2Schema)
        {
            K2Schema->SplitPin(ActionValuePin, false);
        }
    }

    // Notify changes
    Graph->NotifyGraphChanged();
    FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);

    UE_LOG(LogTemp, Display, TEXT("HandleAddEnhancedInputActionEvent: Created node for '%s' (ID: %s)"),
        *InputActionPath, *ActionNode->NodeGuid.ToString());

    // Build response
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("node_id"), ActionNode->NodeGuid.ToString());
    Response->SetStringField(TEXT("input_action"), InputActionPath);
    Response->SetNumberField(TEXT("pos_x"), ActionNode->NodePosX);
    Response->SetNumberField(TEXT("pos_y"), ActionNode->NodePosY);
    Response->SetBoolField(TEXT("reused_existing"), false);

    // List output pins
    TArray<TSharedPtr<FJsonValue>> PinArray;
    for (UEdGraphPin* Pin : ActionNode->Pins)
    {
        if (Pin->Direction == EGPD_Output)
        {
            TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
            PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
            PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
            PinArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
        }
    }
    Response->SetArrayField(TEXT("output_pins"), PinArray);

    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params)
{
    // Required params
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: action_name"));
    }

    FString ActionPath;
    if (!Params->TryGetStringField(TEXT("action_path"), ActionPath))
    {
        ActionPath = TEXT("/Game/Input/Actions/");
    }

    FString ValueTypeStr;
    if (!Params->TryGetStringField(TEXT("value_type"), ValueTypeStr))
    {
        ValueTypeStr = TEXT("Bool");
    }

    // Determine value type
    EInputActionValueType ValueType = EInputActionValueType::Boolean;
    if (ValueTypeStr.Equals(TEXT("Axis1D"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
    {
        ValueType = EInputActionValueType::Axis1D;
    }
    else if (ValueTypeStr.Equals(TEXT("Axis2D"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("Vector2D"), ESearchCase::IgnoreCase))
    {
        ValueType = EInputActionValueType::Axis2D;
    }
    else if (ValueTypeStr.Equals(TEXT("Axis3D"), ESearchCase::IgnoreCase) || ValueTypeStr.Equals(TEXT("Vector3D"), ESearchCase::IgnoreCase))
    {
        ValueType = EInputActionValueType::Axis3D;
    }

    // Build full path
    FString FullPath = ActionPath;
    if (!FullPath.EndsWith(TEXT("/")))
    {
        FullPath += TEXT("/");
    }
    FullPath += ActionName;

    // Check if already exists
    UInputAction* ExistingAction = LoadObject<UInputAction>(nullptr, *FullPath);
    if (!ExistingAction)
    {
        ExistingAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(FullPath));
    }
    if (ExistingAction)
    {
        // Return existing asset info
        TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
        Response->SetBoolField(TEXT("success"), true);
        Response->SetStringField(TEXT("action_name"), ActionName);
        Response->SetStringField(TEXT("action_path"), FullPath);
        Response->SetStringField(TEXT("value_type"), ValueTypeStr);
        Response->SetBoolField(TEXT("already_existed"), true);
        return Response;
    }

    // Create the package
    FString PackagePath = FullPath;
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
    }

    // Create the InputAction asset
    UInputAction* NewAction = NewObject<UInputAction>(Package, UInputAction::StaticClass(), *ActionName, RF_Public | RF_Standalone);
    if (!NewAction)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create InputAction object"));
    }

    NewAction->ValueType = ValueType;

    // Mark dirty and notify asset registry
    FAssetRegistryModule::AssetCreated(NewAction);
    Package->MarkPackageDirty();

    // Save the package
    FString PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, NewAction, *PackageFilename, SaveArgs);

    UE_LOG(LogTemp, Display, TEXT("CreateInputAction: Created '%s' at '%s' (ValueType: %s)"),
        *ActionName, *FullPath, *ValueTypeStr);

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("action_name"), ActionName);
    Response->SetStringField(TEXT("action_path"), FullPath);
    Response->SetStringField(TEXT("value_type"), ValueTypeStr);
    Response->SetBoolField(TEXT("already_existed"), false);
    return Response;
}

TSharedPtr<FJsonObject> FEpicUnrealMCPBlueprintGraphCommands::HandleAddInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    // Required params
    FString ContextPath;
    if (!Params->TryGetStringField(TEXT("context_path"), ContextPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: context_path"));
    }

    FString ActionPath;
    if (!Params->TryGetStringField(TEXT("action_path"), ActionPath))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: action_path"));
    }

    FString KeyName;
    if (!Params->TryGetStringField(TEXT("key"), KeyName))
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: key"));
    }

    // Load or auto-create the Input Mapping Context
    UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, *ContextPath);
    if (!IMC)
    {
        IMC = Cast<UInputMappingContext>(UEditorAssetLibrary::LoadAsset(ContextPath));
    }
    if (!IMC)
    {
        // Auto-create the IMC if it doesn't exist
        FString IMCName = FPaths::GetBaseFilename(ContextPath);
        UPackage* IMCPackage = CreatePackage(*ContextPath);
        if (IMCPackage)
        {
            IMC = NewObject<UInputMappingContext>(IMCPackage, UInputMappingContext::StaticClass(), *IMCName, RF_Public | RF_Standalone);
            if (IMC)
            {
                FAssetRegistryModule::AssetCreated(IMC);
                IMCPackage->MarkPackageDirty();
                FString IMCFilename = FPackageName::LongPackageNameToFilename(ContextPath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(IMCPackage, IMC, *IMCFilename, SaveArgs);
                UE_LOG(LogTemp, Display, TEXT("AddInputMapping: Auto-created InputMappingContext '%s'"), *ContextPath);
            }
        }
        if (!IMC)
        {
            return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputMappingContext not found and could not be created: %s"), *ContextPath));
        }
    }

    // Load the Input Action
    UInputAction* InputAction = LoadObject<UInputAction>(nullptr, *ActionPath);
    if (!InputAction)
    {
        InputAction = Cast<UInputAction>(UEditorAssetLibrary::LoadAsset(ActionPath));
    }
    if (!InputAction)
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputAction not found: %s"), *ActionPath));
    }

    // Parse the key
    FKey Key(*KeyName);
    if (!Key.IsValid())
    {
        return FEpicUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key name: %s. Use Unreal key names like: SpaceBar, LeftShift, Insert, Delete, End, LeftMouseButton, RightMouseButton, A, B, W, S, etc."), *KeyName));
    }

    // Check for optional modifiers
    bool bNegate = false;
    Params->TryGetBoolField(TEXT("negate"), bNegate);

    bool bSwizzle = false;
    Params->TryGetBoolField(TEXT("swizzle"), bSwizzle);

    // Add the mapping - MapKey returns by value in UE5.5
    IMC->MapKey(InputAction, Key);

    // Find the mapping we just added (last one for this action+key combo)
    TArray<FEnhancedActionKeyMapping>& Mappings = const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings());
    FEnhancedActionKeyMapping* Mapping = nullptr;
    for (int32 i = Mappings.Num() - 1; i >= 0; --i)
    {
        if (Mappings[i].Action == InputAction && Mappings[i].Key == Key)
        {
            Mapping = &Mappings[i];
            break;
        }
    }

    if (Mapping)
    {
        // Add modifiers if requested
        if (bNegate)
        {
            UInputModifierNegate* NegateModifier = NewObject<UInputModifierNegate>(IMC);
            Mapping->Modifiers.Add(NegateModifier);
        }

        if (bSwizzle)
        {
            UInputModifierSwizzleAxis* SwizzleModifier = NewObject<UInputModifierSwizzleAxis>(IMC);
            SwizzleModifier->Order = EInputAxisSwizzle::YXZ;
            Mapping->Modifiers.Add(SwizzleModifier);
        }

        // Optional trigger type
        FString TriggerType;
        if (Params->TryGetStringField(TEXT("trigger"), TriggerType))
        {
            if (TriggerType.Equals(TEXT("Pressed"), ESearchCase::IgnoreCase))
            {
                UInputTriggerPressed* Trigger = NewObject<UInputTriggerPressed>(IMC);
                Mapping->Triggers.Add(Trigger);
            }
            else if (TriggerType.Equals(TEXT("Released"), ESearchCase::IgnoreCase))
            {
                UInputTriggerReleased* Trigger = NewObject<UInputTriggerReleased>(IMC);
                Mapping->Triggers.Add(Trigger);
            }
            else if (TriggerType.Equals(TEXT("Hold"), ESearchCase::IgnoreCase))
            {
                UInputTriggerHold* Trigger = NewObject<UInputTriggerHold>(IMC);
                Mapping->Triggers.Add(Trigger);
            }
        }
    }

    // Save the IMC package
    IMC->GetPackage()->MarkPackageDirty();
    FString PackageFilename = FPackageName::LongPackageNameToFilename(
        IMC->GetPackage()->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(IMC->GetPackage(), IMC, *PackageFilename, SaveArgs);

    UE_LOG(LogTemp, Display, TEXT("AddInputMapping: Mapped '%s' -> '%s' in '%s'"),
        *KeyName, *ActionPath, *ContextPath);

    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject());
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("context_path"), ContextPath);
    Response->SetStringField(TEXT("action_path"), ActionPath);
    Response->SetStringField(TEXT("key"), KeyName);
    if (bNegate) Response->SetBoolField(TEXT("negate"), true);
    if (bSwizzle) Response->SetBoolField(TEXT("swizzle"), true);
    return Response;
}
