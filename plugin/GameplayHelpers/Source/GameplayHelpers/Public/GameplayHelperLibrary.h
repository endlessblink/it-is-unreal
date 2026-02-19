// Runtime Blueprint Function Library - Helper functions for gameplay logic

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayHelperLibrary.generated.h"

class AActor;
class ACharacter;
class UAnimSequence;
class UInputMappingContext;

/**
 * Static helper functions for common gameplay operations.
 * These are RUNTIME functions (not editor-only) so they work in packaged builds.
 */
UCLASS()
class GAMEPLAYHELPERS_API UGameplayHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Set the character's max walk speed at runtime.
	 * Wraps CharacterMovementComponent->MaxWalkSpeed = NewSpeed.
	 */
	UFUNCTION(BlueprintCallable, Category="Gameplay|Character", meta=(DefaultToSelf="Character"))
	static void SetCharacterWalkSpeed(ACharacter* Character, float NewSpeed);

	/**
	 * Play an AnimSequence as a one-shot dynamic montage on the character.
	 * Blends in/out smoothly and returns to AnimBP state machine when done.
	 * Uses the DefaultSlot so multiple calls interrupt each other (no stacking).
	 * bForceInterrupt: if true, instantly stops any playing montage first (use for hit-react/death).
	 */
	UFUNCTION(BlueprintCallable, Category="Gameplay|Animation", meta=(DefaultToSelf="Character"))
	static void PlayAnimationOneShot(ACharacter* Character, UAnimSequence* AnimSequence, float PlayRate = 1.0f, float BlendIn = 0.25f, float BlendOut = 0.25f, bool bStopMovement = false, bool bForceInterrupt = false);

	/**
	 * Add an Input Mapping Context to the character's player controller.
	 * Call from BeginPlay to ensure Enhanced Input actions work.
	 */
	UFUNCTION(BlueprintCallable, Category="Gameplay|Input")
	static void AddInputMappingContextToCharacter(ACharacter* Character, UInputMappingContext* MappingContext, int32 Priority = 0);

	/**
	 * Melee damage sweep: sphere overlap around attacker, damage Characters
	 * with a "Health" float variable, ragdoll + knockback + delayed destroy on death.
	 */
	UFUNCTION(BlueprintCallable, Category="Gameplay|Combat", meta=(DefaultToSelf="Attacker"))
	static void ApplyMeleeDamage(ACharacter* Attacker, float Damage = 15.0f, float Radius = 200.0f, float KnockbackImpulse = 50000.0f);

	/**
	 * Tick-based enemy AI: chase player, attack in range, return when leashed.
	 * Locomotion driven by AnimBP (reads CMC velocity). One-shots use montages.
	 * State stored internally (static TMap). Call from Event Tick on each enemy.
	 */
	UFUNCTION(BlueprintCallable, Category="Gameplay|AI", meta=(DefaultToSelf="Enemy"))
	static void UpdateEnemyAI(
		ACharacter* Enemy,
		float AggroRange = 1500.0f,
		float AttackRange = 260.0f,
		float LeashDistance = 3000.0f,
		float MoveSpeed = 400.0f,
		float AttackCooldown = 2.0f,
		float AttackDamage = 10.0f,
		float AttackRadius = 150.0f,
		UAnimSequence* AttackAnim = nullptr,
		UAnimSequence* DeathAnim = nullptr,
		UAnimSequence* HitReactAnim = nullptr,
		UAnimSequence* AttackAnim2 = nullptr,
		UAnimSequence* AttackAnim3 = nullptr,
		UAnimSequence* ScreamAnim = nullptr,
		UAnimSequence* DeathAnim2 = nullptr,
		bool bIgnorePlayer = false,
		float PatrolRadius = 0.0f,
		AActor* CombatPartner = nullptr
	);

	/**
	 * Set whether the player character is currently blocking.
	 * While blocking, incoming damage is reduced by 75%.
	 */
	UFUNCTION(BlueprintCallable, Category="Gameplay|Combat", meta=(DefaultToSelf="Character"))
	static void SetPlayerBlocking(ACharacter* Character, bool bBlocking);

	/**
	 * Check if the player character is currently blocking.
	 */
	UFUNCTION(BlueprintPure, Category="Gameplay|Combat", meta=(DefaultToSelf="Character"))
	static bool IsPlayerBlocking(ACharacter* Character);
};
