#include "EnemyAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"

void UEnemyAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	APawn* Owner = TryGetPawnOwner();
	if (ACharacter* Char = Cast<ACharacter>(Owner))
	{
		CachedCharacter = Char;
		CachedCMC = Char->GetCharacterMovement();
	}

	// One-shot diagnostic: verify LocSpeed property exists on generated class
	FProperty* LocSpeedProp = GetClass()->FindPropertyByName(FName("LocSpeed"));
	UE_LOG(LogTemp, Warning, TEXT("EnemyAnimInstance INIT: Owner=%s Class=%s LocSpeedProp=%s"),
		Owner ? *Owner->GetName() : TEXT("NULL"),
		*GetClass()->GetName(),
		LocSpeedProp ? TEXT("FOUND") : TEXT("NOT_FOUND"));
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (bIsDead)
	{
		return; // Pose is frozen
	}

	if (!CachedCMC.IsValid())
	{
		// Re-cache if character was re-possessed or component changed
		APawn* Owner = TryGetPawnOwner();
		if (ACharacter* Char = Cast<ACharacter>(Owner))
		{
			CachedCharacter = Char;
			CachedCMC = Char->GetCharacterMovement();
		}

		if (!CachedCMC.IsValid())
		{
			return;
		}
	}

	// Target speed from CMC ground velocity (ignore Z for jumps/falls)
	const float TargetSpeed = CachedCMC->Velocity.Size2D();

	// Surgical loop fix: stabilize speed to prevent rapid Idle<->Walk/Run re-entry.
	// Fast rise keeps responsiveness, slower decay filters brief zero-velocity spikes.
	const float RiseInterp = 18.0f;
	const float FallInterp = 6.0f;
	const float Interp = (TargetSpeed >= AnimSpeed) ? RiseInterp : FallInterp;
	AnimSpeed = FMath::FInterpTo(AnimSpeed, TargetSpeed, DeltaSeconds, Interp);

	// Dead-zone snap: only fully stop when close to zero.
	if (AnimSpeed < 2.5f)
	{
		AnimSpeed = 0.0f;
	}

	// Write both Speed and LocSpeed via reflection so BP state machines and
	// BlendSpaces can bind whichever variable they use.
	for (const FName PropName : { FName("Speed"), FName("LocSpeed") })
	{
		if (FProperty* Prop = GetClass()->FindPropertyByName(PropName))
		{
			void* ValPtr = Prop->ContainerPtrToValuePtr<void>(this);
			if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
			{
				FP->SetPropertyValue(ValPtr, AnimSpeed);
			}
			else if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
			{
				DP->SetPropertyValue(ValPtr, (double)AnimSpeed);
			}
		}
	}
}
