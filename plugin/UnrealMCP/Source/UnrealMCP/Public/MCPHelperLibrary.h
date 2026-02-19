// Blueprint Function Library - Helper functions accessible from Blueprint graphs via MCP

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MCPHelperLibrary.generated.h"

/**
 * Static helper functions for MCP-driven Blueprint wiring.
 * These wrap engine functionality that's hard to access via K2Node creation.
 * Use target_class="MCPHelperLibrary" in add_node to reference these.
 */
UCLASS()
class UMCPHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Set the character's max walk speed at runtime.
	 * Wraps CharacterMovementComponent->MaxWalkSpeed = NewSpeed.
	 */
	UFUNCTION(BlueprintCallable, Category="MCP|Character", meta=(DefaultToSelf="Character"))
	static void SetCharacterWalkSpeed(ACharacter* Character, float NewSpeed);

	/**
	 * Play an AnimSequence as a one-shot dynamic montage on the character.
	 * Blends in/out smoothly and returns to AnimBP state machine when done.
	 * Uses the DefaultSlot so multiple calls interrupt each other (no stacking).
	 */
	UFUNCTION(BlueprintCallable, Category="MCP|Animation", meta=(DefaultToSelf="Character"))
	static void PlayAnimationOneShot(ACharacter* Character, UAnimSequence* AnimSequence, float PlayRate = 1.0f, float BlendIn = 0.25f, float BlendOut = 0.25f, bool bStopMovement = false);
};
