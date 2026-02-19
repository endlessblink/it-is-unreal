#include "EpicUnrealMCPModule.h"
#include "EpicUnrealMCPBridge.h"
#include "Modules/ModuleManager.h"
#include "EditorSubsystem.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FEpicUnrealMCPModule"

void FEpicUnrealMCPModule::StartupModule()
{
	UE_LOG(LogTemp, Warning, TEXT("=== UnrealMCP v1.0.0 LOADED ==="));
	UE_LOG(LogTemp, Display, TEXT("Epic Unreal MCP Module has started"));
}

void FEpicUnrealMCPModule::ShutdownModule()
{
	UE_LOG(LogTemp, Display, TEXT("Epic Unreal MCP Module has shut down"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEpicUnrealMCPModule, UnrealMCP) 