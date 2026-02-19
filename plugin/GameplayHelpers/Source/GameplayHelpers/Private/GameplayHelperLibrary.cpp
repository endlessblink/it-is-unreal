#include "GameplayHelperLibrary.h"
#include "EnemyAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "TimerManager.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

// --- Blocking State ---
static TSet<TWeakObjectPtr<AActor>> BlockingActors;

// --- Enemy AI State ---

enum class EEnemyAIState : uint8
{
	Idle,
	Chase,
	Attack,
	Return,
	HitReact,
	Dead,
	Patrol
};

enum class EEnemyPersonality : uint8
{
	Normal,
	Berserker,
	Stalker,
	Brute,
	Crawler
};

enum class EIdleBehavior : uint8
{
	Stand,
	LookAround,
	Wander,
	Scream
};

struct FEnemyAIStateData
{
	FVector SpawnLocation;
	double LastAttackTime = 0.0;
	double HitReactStartTime = 0.0;
	double LastHitReactEndTime = 0.0;
	double DeathStartTime = 0.0;
	float PreviousHealth = -1.f;
	EEnemyAIState CurrentState = EEnemyAIState::Idle;
	EEnemyAIState PreHitReactState = EEnemyAIState::Idle;
	bool bInitialized = false;
	bool bHealthInitialized = false;
	bool bDeathAnimStarted = false;

	// Per-instance randomization
	float SpeedMultiplier = 1.0f;
	float AggroRangeMultiplier = 1.0f;
	float ReactionDelay = 0.0f;
	float AttackCooldownJitter = 0.0f;
	float WobblePhase = 0.0f;
	float WobbleAmplitude = 0.0f;
	float AnimPlayRateVariation = 1.0f;
	double AggroStartTime = 0.0;
	bool bAggroReactionDone = false;

	// Personality archetype
	EEnemyPersonality Personality = EEnemyPersonality::Normal;
	float DamageMultiplier = 1.0f;

	// Per-instance selected animations
	UAnimSequence* ChosenAttackAnim = nullptr;
	UAnimSequence* ChosenDeathAnim = nullptr;
	UAnimSequence* ChosenHitReactAnim = nullptr;

	// Idle behavior
	EIdleBehavior CurrentIdleBehavior = EIdleBehavior::Stand;
	float IdleBehaviorTimer = 0.0f;
	float NextIdleBehaviorTime = 0.0f;
	FVector IdleWanderTarget = FVector::ZeroVector;
	bool bIdleBehaviorActive = false;
	float IdleScreamEndTime = 0.0f;

	// Patrol behavior
	FVector PatrolTarget = FVector::ZeroVector;
	float PatrolPauseTimer = 0.0f;
	float PatrolPauseDuration = 0.0f;
	bool bPatrolPausing = false;

	// Combat partner
	double LastPartnerAttackTime = 0.0;
	float PartnerAttackCooldown = 0.0f;
	TWeakObjectPtr<AActor> AutoDiscoveredPartner;
	bool bPartnerSearchDone = false;

	bool bPendingDamage = false;
	double PendingDamageTime = 0.0;
	float PendingDamageAmount = 0.f;
	float PendingDamageRadius = 0.f;
};

static TMap<TWeakObjectPtr<AActor>, FEnemyAIStateData> EnemyAIStates;

// ============================================================================
// SetCharacterWalkSpeed
// ============================================================================

void UGameplayHelperLibrary::SetCharacterWalkSpeed(ACharacter* Character, float NewSpeed)
{
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		return;
	}

	MovementComp->MaxWalkSpeed = NewSpeed;
}

// ============================================================================
// PlayAnimationOneShot
// ============================================================================

void UGameplayHelperLibrary::PlayAnimationOneShot(ACharacter* Character, UAnimSequence* AnimSequence, float PlayRate, float BlendIn, float BlendOut, bool bStopMovement, bool bForceInterrupt)
{
	if (!Character || !AnimSequence)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
	if (!AnimInst)
	{
		return;
	}

	if (bForceInterrupt)
	{
		AnimInst->Montage_Stop(0.0f);
	}
	else if (AnimInst->Montage_IsPlaying(nullptr))
	{
		return;
	}

	UCharacterMovementComponent* MovementComp = nullptr;
	float SavedSpeed = 0.0f;
	if (bStopMovement)
	{
		MovementComp = Character->GetCharacterMovement();
		if (MovementComp)
		{
			SavedSpeed = MovementComp->MaxWalkSpeed;
			MovementComp->MaxWalkSpeed = 0.0f;
			MovementComp->StopMovementImmediately();
		}
	}

	UAnimMontage* Montage = AnimInst->PlaySlotAnimationAsDynamicMontage(
		AnimSequence,
		FName("DefaultSlot"),
		BlendIn,
		BlendOut,
		PlayRate,
		1,
		-1.0f,
		0.0f
	);

	if (bStopMovement && MovementComp)
	{
		float Duration = AnimSequence->GetPlayLength() / FMath::Max(PlayRate, 0.01f);
		float RestoreTime = FMath::Max(Duration - BlendOut, 0.1f);

		FTimerHandle TimerHandle;
		TWeakObjectPtr<UCharacterMovementComponent> WeakMoveComp(MovementComp);
		Character->GetWorldTimerManager().SetTimer(
			TimerHandle,
			[WeakMoveComp, SavedSpeed]()
			{
				if (WeakMoveComp.IsValid())
				{
					WeakMoveComp->MaxWalkSpeed = SavedSpeed;
				}
			},
			RestoreTime,
			false
		);
	}
}

// ============================================================================
// AddInputMappingContextToCharacter
// ============================================================================

void UGameplayHelperLibrary::AddInputMappingContextToCharacter(ACharacter* Character, UInputMappingContext* MappingContext, int32 Priority)
{
	if (!Character || !MappingContext)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	Subsystem->AddMappingContext(MappingContext, Priority);

	PC->SetInputMode(FInputModeGameOnly());
	PC->bShowMouseCursor = false;
}

// ============================================================================
// SetPlayerBlocking / IsPlayerBlocking
// ============================================================================

void UGameplayHelperLibrary::SetPlayerBlocking(ACharacter* Character, bool bBlocking)
{
	if (!Character) return;

	TWeakObjectPtr<AActor> Key(Character);
	if (bBlocking)
	{
		BlockingActors.Add(Key);
	}
	else
	{
		BlockingActors.Remove(Key);
	}
}

bool UGameplayHelperLibrary::IsPlayerBlocking(ACharacter* Character)
{
	if (!Character) return false;
	return BlockingActors.Contains(TWeakObjectPtr<AActor>(Character));
}

// ============================================================================
// ApplyMeleeDamage
// ============================================================================

void UGameplayHelperLibrary::ApplyMeleeDamage(ACharacter* Attacker, float Damage, float Radius, float KnockbackImpulse)
{
	if (!Attacker)
	{
		return;
	}

	UWorld* World = Attacker->GetWorld();
	if (!World)
	{
		return;
	}

	// Sphere overlap to find nearby pawns
	FVector Origin = Attacker->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Attacker);

	World->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, Sphere, Params);

	// Deduplicate actors
	TSet<ACharacter*> HitCharacters;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || HitActor == Attacker)
		{
			continue;
		}
		ACharacter* HitChar = Cast<ACharacter>(HitActor);
		if (HitChar)
		{
			HitCharacters.Add(HitChar);
		}
	}

	// Forward cone filter (~160 degree cone)
	{
		FVector Fwd = Attacker->GetActorForwardVector();
		Fwd.Z = 0.f;
		if (!Fwd.IsNearlyZero())
		{
			Fwd.Normalize();
			TSet<ACharacter*> InCone;
			for (ACharacter* HC : HitCharacters)
			{
				FVector ToTarget = HC->GetActorLocation() - Origin;
				ToTarget.Z = 0.f;
				float Dist = ToTarget.Size();
				if (Dist < 1.f) { InCone.Add(HC); continue; }
				FVector Dir = ToTarget / Dist;
				if (FVector::DotProduct(Fwd, Dir) > 0.17f)
				{
					InCone.Add(HC);
				}
			}
			HitCharacters = InCone;
		}
	}

	// Prevent friendly fire
	ACharacter* PlayerCharDmg = UGameplayStatics::GetPlayerCharacter(World, 0);
	bool bAttackerIsPlayer = (Attacker == PlayerCharDmg);

	// Player melee hits only closest target
	if (bAttackerIsPlayer && HitCharacters.Num() > 1)
	{
		ACharacter* Closest = nullptr;
		float ClosestDistSq = FLT_MAX;
		for (ACharacter* HC : HitCharacters)
		{
			float DSq = FVector::DistSquared(Origin, HC->GetActorLocation());
			if (DSq < ClosestDistSq)
			{
				ClosestDistSq = DSq;
				Closest = HC;
			}
		}
		HitCharacters.Empty();
		if (Closest) HitCharacters.Add(Closest);
	}

	for (ACharacter* Victim : HitCharacters)
	{
		if (!IsValid(Victim))
		{
			continue;
		}

		bool bVictimIsPlayer = (Victim == PlayerCharDmg);
		if (!bAttackerIsPlayer && !bVictimIsPlayer)
		{
			continue;
		}

		// Find "Health" property via reflection
		FProperty* HealthProp = Victim->GetClass()->FindPropertyByName(FName("Health"));
		if (!HealthProp)
		{
			continue;
		}

		void* ValuePtr = HealthProp->ContainerPtrToValuePtr<void>(Victim);
		float CurrentHealth = 0.0f;

		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(HealthProp))
		{
			CurrentHealth = FloatProp->GetPropertyValue(ValuePtr);
		}
		else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(HealthProp))
		{
			CurrentHealth = (float)DoubleProp->GetPropertyValue(ValuePtr);
		}
		else
		{
			continue;
		}

		if (CurrentHealth <= 0.0f)
		{
			continue;
		}

		// Apply damage (with blocking reduction)
		float EffectiveDamage = Damage;
		if (BlockingActors.Contains(TWeakObjectPtr<AActor>(Victim)))
		{
			EffectiveDamage = Damage * 0.25f;
		}
		CurrentHealth -= EffectiveDamage;

		// Write back
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(HealthProp))
		{
			FloatProp->SetPropertyValue(ValuePtr, CurrentHealth);
		}
		else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(HealthProp))
		{
			DoubleProp->SetPropertyValue(ValuePtr, (double)CurrentHealth);
		}

		if (CurrentHealth <= 0.0f)
		{
			// Death handling
			TWeakObjectPtr<AActor> VictimKey(Victim);
			bool bManagedByAI = EnemyAIStates.Contains(VictimKey);

			if (bManagedByAI)
			{
				// Let UpdateEnemyAI handle death on next tick
				UCapsuleComponent* Capsule = Victim->GetCapsuleComponent();
				if (Capsule)
				{
					Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
				}
			}
			else
			{
				// Ragdoll death path
				USkeletalMeshComponent* MeshComp = Victim->GetMesh();
				if (MeshComp)
				{
					MeshComp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
					MeshComp->SetSimulatePhysics(true);

					FVector KnockDir = (Victim->GetActorLocation() - Origin).GetSafeNormal();
					KnockDir.Z = 0.3f;
					KnockDir.Normalize();
					MeshComp->AddImpulse(KnockDir * KnockbackImpulse);
				}

				UCapsuleComponent* Capsule = Victim->GetCapsuleComponent();
				if (Capsule)
				{
					Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}

				Victim->GetCharacterMovement()->DisableMovement();

				// Player death: disable input (game-specific HUD should handle restart)
				if (Victim == PlayerCharDmg)
				{
					APlayerController* PC = Cast<APlayerController>(Victim->GetController());
					if (PC)
					{
						PC->DisableInput(PC);
					}
				}
				else
				{
					// Enemy: delayed destroy
					TWeakObjectPtr<ACharacter> WeakVictim(Victim);
					FTimerHandle DestroyTimer;
					World->GetTimerManager().SetTimer(
						DestroyTimer,
						[WeakVictim]()
						{
							if (WeakVictim.IsValid())
							{
								WeakVictim->Destroy();
							}
						},
						1.5f,
						false
					);
				}
			}
		}
	}
}

// ============================================================================
// UpdateEnemyAI
// ============================================================================

void UGameplayHelperLibrary::UpdateEnemyAI(ACharacter* Enemy, float AggroRange, float AttackRange,
	float LeashDistance, float MoveSpeed, float AttackCooldown,
	float AttackDamage, float AttackRadius, UAnimSequence* AttackAnim,
	UAnimSequence* DeathAnim, UAnimSequence* HitReactAnim,
	UAnimSequence* AttackAnim2, UAnimSequence* AttackAnim3,
	UAnimSequence* ScreamAnim, UAnimSequence* DeathAnim2,
	bool bIgnorePlayer, float PatrolRadius, AActor* CombatPartner)
{
	if (!Enemy) return;
	UWorld* World = Enemy->GetWorld();
	if (!World) return;

	// Get/init state
	TWeakObjectPtr<AActor> Key(Enemy);
	FEnemyAIStateData& State = EnemyAIStates.FindOrAdd(Key);
	if (!State.bInitialized)
	{
		State.bInitialized = true;

		// Per-instance randomization
		State.SpeedMultiplier = FMath::FRandRange(0.85f, 1.15f);
		State.AggroRangeMultiplier = FMath::FRandRange(0.7f, 1.3f);
		State.ReactionDelay = FMath::FRandRange(0.1f, 1.5f);
		State.AttackCooldownJitter = FMath::FRandRange(-0.5f, 1.0f);
		State.WobblePhase = FMath::FRandRange(0.0f, 2.0f * PI);
		State.WobbleAmplitude = FMath::FRandRange(30.0f, 80.0f);
		State.AnimPlayRateVariation = FMath::FRandRange(0.8f, 1.2f);

		// Personality archetype
		{
			float PersonalityRoll = FMath::FRand();
			if (PersonalityRoll < 0.30f)
				State.Personality = EEnemyPersonality::Normal;
			else if (PersonalityRoll < 0.45f)
				State.Personality = EEnemyPersonality::Berserker;
			else if (PersonalityRoll < 0.65f)
				State.Personality = EEnemyPersonality::Stalker;
			else if (PersonalityRoll < 0.80f)
				State.Personality = EEnemyPersonality::Brute;
			else
				State.Personality = EEnemyPersonality::Crawler;

			switch (State.Personality)
			{
			case EEnemyPersonality::Berserker:
				State.SpeedMultiplier *= 1.2f;
				State.AggroRangeMultiplier *= 0.5f;
				State.AttackCooldownJitter -= 1.0f;
				State.ReactionDelay *= 0.2f;
				State.DamageMultiplier = 0.7f;
				break;
			case EEnemyPersonality::Stalker:
				State.SpeedMultiplier *= 0.8f;
				State.AggroRangeMultiplier *= 2.0f;
				State.ReactionDelay *= 2.5f;
				State.WobbleAmplitude *= 2.0f;
				State.DamageMultiplier = 1.0f;
				break;
			case EEnemyPersonality::Brute:
				State.SpeedMultiplier *= 0.9f;
				State.WobbleAmplitude *= 0.2f;
				State.AttackCooldownJitter += 0.5f;
				State.DamageMultiplier = 1.8f;
				break;
			case EEnemyPersonality::Crawler:
				State.SpeedMultiplier *= 0.75f;
				State.AggroRangeMultiplier *= 1.4f;
				State.ReactionDelay *= 0.5f;
				State.AnimPlayRateVariation *= 0.8f;
				State.DamageMultiplier = 1.2f;
				break;
			default:
				State.DamageMultiplier = 1.0f;
				break;
			}

			// Select attack animation from available pool
			TArray<UAnimSequence*> AvailableAttacks;
			if (AttackAnim) AvailableAttacks.Add(AttackAnim);
			if (AttackAnim2) AvailableAttacks.Add(AttackAnim2);
			if (AttackAnim3) AvailableAttacks.Add(AttackAnim3);

			if (AvailableAttacks.Num() > 0)
			{
				switch (State.Personality)
				{
				case EEnemyPersonality::Stalker:
					State.ChosenAttackAnim = AvailableAttacks.Num() > 1
						? AvailableAttacks[1] : AvailableAttacks[0];
					break;
				case EEnemyPersonality::Brute:
					State.ChosenAttackAnim = AvailableAttacks.Last();
					break;
				default:
					State.ChosenAttackAnim = AvailableAttacks[FMath::RandRange(0, AvailableAttacks.Num() - 1)];
					break;
				}
			}

			// Select death animation
			TArray<UAnimSequence*> AvailableDeaths;
			if (DeathAnim) AvailableDeaths.Add(DeathAnim);
			if (DeathAnim2) AvailableDeaths.Add(DeathAnim2);
			if (AvailableDeaths.Num() > 0)
				State.ChosenDeathAnim = AvailableDeaths[FMath::RandRange(0, AvailableDeaths.Num() - 1)];

			// Use passed-in hit react anim as default
			State.ChosenHitReactAnim = HitReactAnim;

			State.NextIdleBehaviorTime = FMath::FRandRange(2.0f, 8.0f);
			State.IdleBehaviorTimer = 0.0f;
		}

		// Snap to ground on first tick
		{
			UCapsuleComponent* SnapCapsule = Enemy->GetCapsuleComponent();
			float SnapOffset = SnapCapsule ? SnapCapsule->GetScaledCapsuleHalfHeight() : 90.0f;

			FVector Loc = Enemy->GetActorLocation();
			FHitResult SnapHit;
			FVector SnapStart = FVector(Loc.X, Loc.Y, Loc.Z + 5000.0f);
			FVector SnapEnd = FVector(Loc.X, Loc.Y, Loc.Z - 5000.0f);
			FCollisionQueryParams SnapParams;
			SnapParams.AddIgnoredActor(Enemy);

			if (World->LineTraceSingleByChannel(SnapHit, SnapStart, SnapEnd, ECC_WorldStatic, SnapParams))
			{
				FVector SnappedLoc = FVector(Loc.X, Loc.Y, SnapHit.Location.Z + SnapOffset);
				Enemy->SetActorLocation(SnappedLoc, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}

		State.SpawnLocation = Enemy->GetActorLocation();

		// Configure capsule collision
		UCapsuleComponent* InitCapsule = Enemy->GetCapsuleComponent();
		if (InitCapsule)
		{
			InitCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			InitCapsule->SetCollisionObjectType(ECC_Pawn);
			InitCapsule->SetCollisionResponseToAllChannels(ECR_Block);
			InitCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		}

		// Configure CMC
		UCharacterMovementComponent* InitMoveComp = Enemy->GetCharacterMovement();
		if (InitMoveComp)
		{
			InitMoveComp->SetComponentTickEnabled(true);
			InitMoveComp->GravityScale = 3.0f;
			InitMoveComp->MaxWalkSpeed = MoveSpeed;
			InitMoveComp->MaxAcceleration = 4096.0f;
			InitMoveComp->BrakingDecelerationWalking = 300.0f;
			InitMoveComp->GroundFriction = 6.0f;
			InitMoveComp->MaxStepHeight = 20.0f;
			InitMoveComp->bOrientRotationToMovement = false;
			InitMoveComp->SetAvoidanceEnabled(true);
			InitMoveComp->AvoidanceWeight = 0.5f;
			InitMoveComp->SetMovementMode(MOVE_Walking);
			InitMoveComp->FindFloor(Enemy->GetActorLocation(), InitMoveComp->CurrentFloor, false);
		}

		// Force mesh tick + anim settings
		USkeletalMeshComponent* InitMesh = Enemy->GetMesh();
		if (InitMesh)
		{
			InitMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			InitMesh->bPauseAnims = false;
			InitMesh->SetComponentTickEnabled(true);
		}
	}

	// Clean up dead entries periodically
	static int32 CleanupCounter = 0;
	if (++CleanupCounter > 100)
	{
		CleanupCounter = 0;
		for (auto It = EnemyAIStates.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid()) It.RemoveCurrent();
		}
	}

	// Find player
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (!Player) return;

	float DistToPlayer = FVector::Dist(Enemy->GetActorLocation(), Player->GetActorLocation());
	float DistToSpawn = FVector::Dist2D(Enemy->GetActorLocation(), State.SpawnLocation);
	double CurrentTime = World->GetTimeSeconds();
	float DeltaTime = World->GetDeltaSeconds();

	// Check Health
	FProperty* HealthProp = Enemy->GetClass()->FindPropertyByName(FName("Health"));
	float HP = 100.f;
	if (HealthProp)
	{
		void* ValPtr = HealthProp->ContainerPtrToValuePtr<void>(Enemy);
		HP = 0.f;
		if (FFloatProperty* FP = CastField<FFloatProperty>(HealthProp)) HP = FP->GetPropertyValue(ValPtr);
		else if (FDoubleProperty* DP = CastField<FDoubleProperty>(HealthProp)) HP = (float)DP->GetPropertyValue(ValPtr);

		if (!State.bHealthInitialized)
		{
			State.bHealthInitialized = true;
			// Default HP — override per enemy type in your project
			if (HP <= 0.f || HP == 100.f)
			{
				HP = 100.f;
				if (FFloatProperty* FP = CastField<FFloatProperty>(HealthProp)) FP->SetPropertyValue(ValPtr, HP);
				else if (FDoubleProperty* DP = CastField<FDoubleProperty>(HealthProp)) DP->SetPropertyValue(ValPtr, (double)HP);
			}
		}
	}

	USkeletalMeshComponent* MeshComp = Enemy->GetMesh();
	UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement();
	UCapsuleComponent* LiveCapsule = Enemy->GetCapsuleComponent();
	if (HP > 0.f && LiveCapsule)
	{
		LiveCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	}

	// --- DEATH STATE ---
	if (HP <= 0.f)
	{
		if (!State.bDeathAnimStarted)
		{
			State.bDeathAnimStarted = true;
			State.bPendingDamage = false;
			State.CurrentState = EEnemyAIState::Dead;
			State.DeathStartTime = CurrentTime;

			if (MoveComp) MoveComp->DisableMovement();

			// Cancel any playing montage
			if (MeshComp)
			{
				if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
				{
					AnimInst->Montage_Stop(0.0f);
				}
			}

			// Play death animation
			UAnimSequence* UsedDeathAnim = State.ChosenDeathAnim ? State.ChosenDeathAnim : DeathAnim;
			if (UsedDeathAnim && MeshComp)
			{
				if (UAnimInstance* DeathAnimInst = MeshComp->GetAnimInstance())
				{
					DeathAnimInst->PlaySlotAnimationAsDynamicMontage(
						UsedDeathAnim, FName("DefaultSlot"),
						0.1f, 0.0f, 1.0f, 1, -1.f, 0.0f
					);
				}
			}
			else if (MeshComp)
			{
				MeshComp->bPauseAnims = true;
			}

			// Signal AnimInstance to freeze
			if (MeshComp)
			{
				if (UEnemyAnimInstance* EnemyAnim = Cast<UEnemyAnimInstance>(MeshComp->GetAnimInstance()))
				{
					EnemyAnim->bIsDead = true;
				}
			}
		}
		else
		{
			// Wait for death anim, then destroy
			UAnimSequence* UsedDeathAnim = State.ChosenDeathAnim ? State.ChosenDeathAnim : DeathAnim;
			float DeathAnimLen = UsedDeathAnim ? UsedDeathAnim->GetPlayLength() : 0.0f;
			float TimeSinceDeath = (float)(CurrentTime - State.DeathStartTime);

			// Freeze pose after death anim
			if (TimeSinceDeath >= DeathAnimLen - 0.05f && MeshComp && !MeshComp->bPauseAnims)
			{
				MeshComp->bPauseAnims = true;
			}

			// Destroy after death anim + settle time
			if (TimeSinceDeath > DeathAnimLen + 2.0f)
			{
				UCapsuleComponent* DeathCapsule = Enemy->GetCapsuleComponent();
				if (DeathCapsule)
				{
					DeathCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
				EnemyAIStates.Remove(Key);
				Enemy->Destroy();
			}
		}
		return;
	}

	// --- HIT REACTION DETECTION ---
	if (State.PreviousHealth > 0.f && HP < State.PreviousHealth && HP > 0.f)
	{
		UAnimSequence* UsedHitReactAnim = State.ChosenHitReactAnim ? State.ChosenHitReactAnim : HitReactAnim;
		bool bStaggerImmune = (State.LastHitReactEndTime > 0.0 && (CurrentTime - State.LastHitReactEndTime) < 0.5);
		if (State.CurrentState != EEnemyAIState::HitReact && !bStaggerImmune)
		{
			State.PreHitReactState = State.CurrentState;
			State.CurrentState = EEnemyAIState::HitReact;
			State.HitReactStartTime = CurrentTime;
			State.bPendingDamage = false;

			if (UsedHitReactAnim)
			{
				PlayAnimationOneShot(Enemy, UsedHitReactAnim, 1.0f, 0.1f, 0.15f, false, true);
			}
			else if (MoveComp)
			{
				MoveComp->StopMovementImmediately();
			}
		}
	}
	State.PreviousHealth = HP;

	// HitReact -> previous state after animation finishes
	if (State.CurrentState == EEnemyAIState::HitReact)
	{
		UAnimSequence* HitReactForLen = State.ChosenHitReactAnim ? State.ChosenHitReactAnim : HitReactAnim;
		float HitReactLen = HitReactForLen ? FMath::Min(HitReactForLen->GetPlayLength(), 0.5f) : 0.5f;
		if ((CurrentTime - State.HitReactStartTime) > HitReactLen)
		{
			State.CurrentState = State.PreHitReactState;
			State.LastHitReactEndTime = CurrentTime;

			// Stop hit-react montage so BlendSpace resumes
			USkeletalMeshComponent* HRMesh = Enemy->GetMesh();
			if (HRMesh)
			{
				if (UAnimInstance* HRAnim = HRMesh->GetAnimInstance())
				{
					HRAnim->Montage_Stop(0.15f);
				}
			}
		}
	}

	// Update CMC walk speed
	if (MoveComp)
	{
		MoveComp->MaxWalkSpeed = MoveSpeed * State.SpeedMultiplier;
	}

	// Process pending damage
	if (State.bPendingDamage && CurrentTime >= State.PendingDamageTime)
	{
		if (State.CurrentState == EEnemyAIState::Attack)
		{
			ApplyMeleeDamage(Enemy, State.PendingDamageAmount, State.PendingDamageRadius, 30000.f);
		}
		State.bPendingDamage = false;
	}

	// --- STATE TRANSITIONS ---
	if (State.CurrentState != EEnemyAIState::HitReact)
	{
		if (DistToSpawn > LeashDistance && State.CurrentState != EEnemyAIState::Return)
		{
			State.CurrentState = EEnemyAIState::Return;
		}

		if (State.CurrentState == EEnemyAIState::Return && DistToSpawn < 150.f)
		{
			State.CurrentState = EEnemyAIState::Idle;
		}

		if (!bIgnorePlayer && State.CurrentState == EEnemyAIState::Idle && DistToPlayer < AggroRange * State.AggroRangeMultiplier)
		{
			State.bIdleBehaviorActive = false;
			State.AggroStartTime = CurrentTime;
			State.bAggroReactionDone = false;
			State.CurrentState = EEnemyAIState::Chase;
		}

		if (State.CurrentState == EEnemyAIState::Idle && PatrolRadius > 0.0f && !State.bIdleBehaviorActive)
		{
			if (State.IdleBehaviorTimer > State.NextIdleBehaviorTime)
			{
				FVector2D RandDir2D = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
				float WanderDist = FMath::FRandRange(PatrolRadius * 0.3f, PatrolRadius);
				State.PatrolTarget = State.SpawnLocation + FVector(RandDir2D.X * WanderDist, RandDir2D.Y * WanderDist, 0.f);
				State.bPatrolPausing = false;
				State.CurrentState = EEnemyAIState::Patrol;
			}
		}

		if (!bIgnorePlayer && State.CurrentState == EEnemyAIState::Patrol && DistToPlayer < AggroRange * State.AggroRangeMultiplier)
		{
			State.AggroStartTime = CurrentTime;
			State.bAggroReactionDone = false;
			State.CurrentState = EEnemyAIState::Chase;
		}

		if (State.CurrentState == EEnemyAIState::Chase && DistToPlayer < AttackRange)
		{
			State.CurrentState = EEnemyAIState::Attack;
			State.LastAttackTime = CurrentTime - (AttackCooldown + State.AttackCooldownJitter) + 0.5;
		}

		if (State.CurrentState == EEnemyAIState::Attack && DistToPlayer > AttackRange * 1.5f)
		{
			State.CurrentState = EEnemyAIState::Chase;
		}

		if (State.CurrentState == EEnemyAIState::Chase && DistToPlayer > AggroRange * State.AggroRangeMultiplier * 1.2f)
		{
			State.CurrentState = EEnemyAIState::Return;
		}
	}

	// --- STATE BEHAVIORS ---
	switch (State.CurrentState)
	{
	case EEnemyAIState::Return:
	{
		FVector DirToSpawn = State.SpawnLocation - Enemy->GetActorLocation();
		FVector HorizDir = FVector(DirToSpawn.X, DirToSpawn.Y, 0.0f).GetSafeNormal();
		if (!HorizDir.IsNearlyZero())
		{
			Enemy->AddMovementInput(HorizDir, 1.0f);
			FRotator TargetRot = FRotator(0.f, HorizDir.Rotation().Yaw, 0.f);
			Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 8.0f));
		}
		break;
	}

	case EEnemyAIState::Attack:
	{
		FVector DirToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
		FVector HorizDir = FVector(DirToPlayer.X, DirToPlayer.Y, 0.0f).GetSafeNormal();
		if (!HorizDir.IsNearlyZero())
		{
			FRotator TargetRot = FRotator(0.f, HorizDir.Rotation().Yaw, 0.f);
			Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 10.0f));
		}

		if ((CurrentTime - State.LastAttackTime) >= (AttackCooldown + State.AttackCooldownJitter))
		{
			State.LastAttackTime = CurrentTime;

			UAnimSequence* UsedAttackAnim = State.ChosenAttackAnim ? State.ChosenAttackAnim : AttackAnim;
			if (UsedAttackAnim)
			{
				PlayAnimationOneShot(Enemy, UsedAttackAnim, 1.0f, 0.15f, 0.2f, false);
			}

			// Queue delayed damage
			State.bPendingDamage = true;
			float WindupDelay = 0.50f;
			if (UsedAttackAnim)
			{
				WindupDelay = FMath::Clamp(UsedAttackAnim->GetPlayLength() * 0.58f, 0.30f, 0.85f);
			}
			State.PendingDamageTime = CurrentTime + WindupDelay;
			State.PendingDamageAmount = AttackDamage * State.DamageMultiplier;
			State.PendingDamageRadius = AttackRadius;
		}
		break;
	}

	case EEnemyAIState::Chase:
	{
		if (!State.bAggroReactionDone)
		{
			if ((CurrentTime - State.AggroStartTime) < State.ReactionDelay)
			{
				FVector DirToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
				FVector HDir = FVector(DirToPlayer.X, DirToPlayer.Y, 0.0f).GetSafeNormal();
				if (!HDir.IsNearlyZero())
				{
					FRotator TargetRot = FRotator(0.f, HDir.Rotation().Yaw, 0.f);
					Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 5.0f));
				}
				break;
			}
			State.bAggroReactionDone = true;
		}

		FVector DirToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
		FVector HorizDir = FVector(DirToPlayer.X, DirToPlayer.Y, 0.0f).GetSafeNormal();
		float HorizDistToPlayer = FVector(DirToPlayer.X, DirToPlayer.Y, 0.0f).Size();
		if (HorizDistToPlayer > AttackRange * 0.8f)
		{
			if (!HorizDir.IsNearlyZero())
			{
				FVector WobbleDir = FVector(-HorizDir.Y, HorizDir.X, 0.0f);
				float WobbleOffset = FMath::Sin((float)CurrentTime * 2.5f + State.WobblePhase) * 0.15f;
				FVector FinalDir = (HorizDir + WobbleDir * WobbleOffset).GetSafeNormal();
				Enemy->AddMovementInput(FinalDir, 1.0f);
				FRotator TargetRot = FRotator(0.f, FinalDir.Rotation().Yaw, 0.f);
				Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 6.0f));
			}
		}
		else
		{
			if (!HorizDir.IsNearlyZero())
			{
				FRotator TargetRot = FRotator(0.f, HorizDir.Rotation().Yaw, 0.f);
				Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 8.0f));
			}
		}
		break;
	}

	case EEnemyAIState::HitReact:
	{
		FVector DirToPlayer = Player->GetActorLocation() - Enemy->GetActorLocation();
		FVector HorizDir = FVector(DirToPlayer.X, DirToPlayer.Y, 0.0f).GetSafeNormal();
		if (!HorizDir.IsNearlyZero())
		{
			FRotator TargetRot = FRotator(0.f, HorizDir.Rotation().Yaw, 0.f);
			Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 6.0f));
		}
		break;
	}

	case EEnemyAIState::Patrol:
	{
		if (State.bPatrolPausing)
		{
			State.PatrolPauseTimer += DeltaTime;
			if (State.PatrolPauseTimer >= State.PatrolPauseDuration)
			{
				State.bPatrolPausing = false;
				State.CurrentState = EEnemyAIState::Idle;
				State.IdleBehaviorTimer = 0.0f;
				State.NextIdleBehaviorTime = FMath::FRandRange(1.0f, 3.0f);
			}
		}
		else
		{
			FVector Dir = State.PatrolTarget - Enemy->GetActorLocation();
			FVector HDir = FVector(Dir.X, Dir.Y, 0.0f);
			float Dist = HDir.Size();

			if (Dist > 80.f)
			{
				FVector Normal = HDir.GetSafeNormal();
				Enemy->AddMovementInput(Normal, 0.4f);
				FRotator TargetRot = FRotator(0.f, Normal.Rotation().Yaw, 0.f);
				Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 3.0f));
			}
			else
			{
				State.bPatrolPausing = true;
				State.PatrolPauseTimer = 0.0f;
				State.PatrolPauseDuration = FMath::FRandRange(2.0f, 5.0f);
			}
		}
		break;
	}

	case EEnemyAIState::Idle:
	default:
	{
		State.IdleBehaviorTimer += DeltaTime;

		// Auto-discover combat partner
		AActor* EffectiveCombatPartner = CombatPartner;
		if (!EffectiveCombatPartner && !State.bPartnerSearchDone)
		{
			State.bPartnerSearchDone = true;
			const float PartnerSearchRadius = 500.0f;
			float BestDist = PartnerSearchRadius;
			for (auto& Pair : EnemyAIStates)
			{
				AActor* Other = Pair.Key.Get();
				if (!Other || Other == Enemy || !IsValid(Other)) continue;
				if (Other->GetClass() != Enemy->GetClass()) continue;
				float Dist = FVector::Dist(Enemy->GetActorLocation(), Other->GetActorLocation());
				if (Dist < BestDist)
				{
					FEnemyAIStateData& OtherState = Pair.Value;
					if (!OtherState.AutoDiscoveredPartner.IsValid())
					{
						BestDist = Dist;
						State.AutoDiscoveredPartner = Other;
					}
				}
			}
			if (State.AutoDiscoveredPartner.IsValid())
			{
				FEnemyAIStateData* OtherData = EnemyAIStates.Find(State.AutoDiscoveredPartner);
				if (OtherData)
				{
					OtherData->AutoDiscoveredPartner = Enemy;
					OtherData->bPartnerSearchDone = true;
				}
			}
		}
		if (!EffectiveCombatPartner && State.AutoDiscoveredPartner.IsValid())
		{
			EffectiveCombatPartner = State.AutoDiscoveredPartner.Get();
		}

		// Combat partner behavior
		if (EffectiveCombatPartner && IsValid(EffectiveCombatPartner))
		{
			FVector DirToPartner = EffectiveCombatPartner->GetActorLocation() - Enemy->GetActorLocation();
			FVector HorizDir = FVector(DirToPartner.X, DirToPartner.Y, 0.0f).GetSafeNormal();
			if (!HorizDir.IsNearlyZero())
			{
				FRotator TargetRot = FRotator(0.f, HorizDir.Rotation().Yaw, 0.f);
				Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 6.0f));
			}

			if (State.PartnerAttackCooldown == 0.0f)
			{
				State.PartnerAttackCooldown = FMath::FRandRange(2.0f, 4.0f);
				State.LastPartnerAttackTime = CurrentTime - FMath::FRandRange(0.0f, State.PartnerAttackCooldown);
			}

			if ((CurrentTime - State.LastPartnerAttackTime) >= State.PartnerAttackCooldown)
			{
				State.LastPartnerAttackTime = CurrentTime;
				State.PartnerAttackCooldown = FMath::FRandRange(2.5f, 5.0f);

				TArray<UAnimSequence*> PartnerAttackPool;
				if (AttackAnim) PartnerAttackPool.Add(AttackAnim);
				if (AttackAnim2) PartnerAttackPool.Add(AttackAnim2);
				if (AttackAnim3) PartnerAttackPool.Add(AttackAnim3);
				UAnimSequence* UsedAttackAnim = PartnerAttackPool.Num() > 0
					? PartnerAttackPool[FMath::RandRange(0, PartnerAttackPool.Num() - 1)] : nullptr;
				if (UsedAttackAnim)
				{
					PlayAnimationOneShot(Enemy, UsedAttackAnim, State.AnimPlayRateVariation, 0.15f, 0.15f, false);
				}
			}

			break;
		}

		// Idle behaviors
		if (!State.bIdleBehaviorActive && State.IdleBehaviorTimer >= State.NextIdleBehaviorTime)
		{
			float Roll = FMath::FRand();
			if (ScreamAnim && Roll < 0.12f)
			{
				State.CurrentIdleBehavior = EIdleBehavior::Scream;
				State.bIdleBehaviorActive = true;
				State.IdleScreamEndTime = CurrentTime + ScreamAnim->GetPlayLength();
				State.IdleBehaviorTimer = 0.0f;
				PlayAnimationOneShot(Enemy, ScreamAnim, 1.0f, 0.15f, 0.15f, false, true);
			}
			else if (Roll < 0.35f)
			{
				State.CurrentIdleBehavior = EIdleBehavior::LookAround;
				State.bIdleBehaviorActive = true;
				State.IdleBehaviorTimer = 0.0f;
			}
			else if (Roll < 0.60f)
			{
				State.CurrentIdleBehavior = EIdleBehavior::Wander;
				State.bIdleBehaviorActive = true;
				State.IdleBehaviorTimer = 0.0f;
				FVector2D RandDir = FVector2D(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
				float WanderDist = FMath::FRandRange(80.f, 250.f);
				State.IdleWanderTarget = State.SpawnLocation + FVector(RandDir.X * WanderDist, RandDir.Y * WanderDist, 0.f);
			}
			else
			{
				State.IdleBehaviorTimer = 0.0f;
				State.NextIdleBehaviorTime = FMath::FRandRange(3.0f, 8.0f);
			}
		}

		if (State.bIdleBehaviorActive)
		{
			switch (State.CurrentIdleBehavior)
			{
			case EIdleBehavior::LookAround:
			{
				float TurnRate = FMath::Sin(State.IdleBehaviorTimer * 1.5f) * 50.0f;
				FRotator CurrentRot = Enemy->GetActorRotation();
				CurrentRot.Yaw += TurnRate * DeltaTime;
				Enemy->SetActorRotation(CurrentRot);

				if (State.IdleBehaviorTimer > 3.5f)
				{
					State.bIdleBehaviorActive = false;
					State.IdleBehaviorTimer = 0.0f;
					State.NextIdleBehaviorTime = FMath::FRandRange(4.0f, 10.0f);
				}
				break;
			}
			case EIdleBehavior::Wander:
			{
				FVector Dir = State.IdleWanderTarget - Enemy->GetActorLocation();
				FVector HDir = FVector(Dir.X, Dir.Y, 0.0f);
				float Dist = HDir.Size();

				if (Dist > 50.f && State.IdleBehaviorTimer < 5.0f)
				{
					FVector Normal = HDir.GetSafeNormal();
					Enemy->AddMovementInput(Normal, 0.4f);
					FRotator TargetRot = FRotator(0.f, Normal.Rotation().Yaw, 0.f);
					Enemy->SetActorRotation(FMath::RInterpTo(Enemy->GetActorRotation(), TargetRot, DeltaTime, 3.0f));
				}
				else
				{
					State.bIdleBehaviorActive = false;
					State.IdleBehaviorTimer = 0.0f;
					State.NextIdleBehaviorTime = FMath::FRandRange(3.0f, 7.0f);
				}
				break;
			}
			case EIdleBehavior::Scream:
			{
				if (CurrentTime > State.IdleScreamEndTime)
				{
					State.bIdleBehaviorActive = false;
					State.IdleBehaviorTimer = 0.0f;
					State.NextIdleBehaviorTime = FMath::FRandRange(8.0f, 15.0f);
				}
				break;
			}
			default:
				State.bIdleBehaviorActive = false;
				break;
			}
		}
		break;
	}
	}
}
