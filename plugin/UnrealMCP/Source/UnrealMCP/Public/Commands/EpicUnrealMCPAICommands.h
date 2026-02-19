#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for AI-related MCP commands
 * Handles behavior tree creation, blackboard setup, task/decorator insertion,
 * and runtime BT assignment to AI controllers
 */
class UNREALMCP_API FEpicUnrealMCPAICommands
{
public:
	FEpicUnrealMCPAICommands();

	// Handle AI commands
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Create a new Behavior Tree asset with a root composite node
	TSharedPtr<FJsonObject> HandleCreateBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	// Create a new Blackboard Data asset with typed keys
	TSharedPtr<FJsonObject> HandleCreateBlackboard(const TSharedPtr<FJsonObject>& Params);

	// Add a task node (MoveTo, Wait, PlayAnimation, RunEQSQuery) to a BT root
	TSharedPtr<FJsonObject> HandleAddBTTask(const TSharedPtr<FJsonObject>& Params);

	// Add a decorator (Blackboard, Cooldown, TimeLimit, IsAtLocation) to a BT child
	TSharedPtr<FJsonObject> HandleAddBTDecorator(const TSharedPtr<FJsonObject>& Params);

	// Assign a behavior tree (and optional blackboard) to an AI-controlled actor
	TSharedPtr<FJsonObject> HandleAssignBehaviorTree(const TSharedPtr<FJsonObject>& Params);
};
