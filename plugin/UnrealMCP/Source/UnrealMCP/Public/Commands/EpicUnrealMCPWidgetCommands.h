#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Widget/UMG-related MCP commands
 * Handles widget blueprint creation, viewport display, and property editing
 */
class UNREALMCP_API FEpicUnrealMCPWidgetCommands
{
public:
	FEpicUnrealMCPWidgetCommands();

	// Handle widget commands
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Widget blueprint creation with optional child elements
	TSharedPtr<FJsonObject> HandleCreateWidgetBlueprint(const TSharedPtr<FJsonObject>& Params);

	// Add widget to viewport (PIE) or validate (editor)
	TSharedPtr<FJsonObject> HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params);

	// Set property on a named child widget inside a widget blueprint
	TSharedPtr<FJsonObject> HandleSetWidgetProperty(const TSharedPtr<FJsonObject>& Params);
};
