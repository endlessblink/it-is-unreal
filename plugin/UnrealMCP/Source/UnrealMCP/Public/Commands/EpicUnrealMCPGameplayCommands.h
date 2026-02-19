#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Gameplay-related MCP commands
 * Handles game mode setup, animation montages, physics impulses,
 * post-process effects, and Niagara particle systems
 */
class UNREALMCP_API FEpicUnrealMCPGameplayCommands
{
public:
	FEpicUnrealMCPGameplayCommands();

	// Handle gameplay commands
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
	// Game mode and pawn configuration
	TSharedPtr<FJsonObject> HandleSetGameModeDefaultPawn(const TSharedPtr<FJsonObject>& Params);

	// Animation montage creation and playback
	TSharedPtr<FJsonObject> HandleCreateAnimMontage(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandlePlayMontageOnActor(const TSharedPtr<FJsonObject>& Params);

	// Physics impulse
	TSharedPtr<FJsonObject> HandleApplyImpulse(const TSharedPtr<FJsonObject>& Params);

	// Post-process runtime effects
	TSharedPtr<FJsonObject> HandleTriggerPostProcessEffect(const TSharedPtr<FJsonObject>& Params);

	// Niagara particle system spawning
	TSharedPtr<FJsonObject> HandleSpawnNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	// Set animation on a placed actor's skeletal mesh (SingleNode mode)
	TSharedPtr<FJsonObject> HandleSetSkeletalAnimation(const TSharedPtr<FJsonObject>& Params);

	// Niagara system asset creation from template emitter
	TSharedPtr<FJsonObject> HandleCreateNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	// Set runtime parameter on a spawned NiagaraActor's component
	TSharedPtr<FJsonObject> HandleSetNiagaraParameter(const TSharedPtr<FJsonObject>& Params);

	// Create atmospheric FX Niagara system with correct module stack
	TSharedPtr<FJsonObject> HandleCreateAtmosphericFX(const TSharedPtr<FJsonObject>& Params);
};
