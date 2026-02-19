#include "MCPHelperLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"

void UMCPHelperLibrary::SetCharacterWalkSpeed(ACharacter* Character, float NewSpeed)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPHelperLibrary::SetCharacterWalkSpeed: Character is null"));
		return;
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPHelperLibrary::SetCharacterWalkSpeed: No CharacterMovementComponent"));
		return;
	}

	MovementComp->MaxWalkSpeed = NewSpeed;
}

void UMCPHelperLibrary::PlayAnimationOneShot(ACharacter* Character, UAnimSequence* AnimSequence, float PlayRate, float BlendIn, float BlendOut, bool bStopMovement)
{
	if (!Character || !AnimSequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPHelperLibrary::PlayAnimationOneShot: Character or AnimSequence is null"));
		return;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPHelperLibrary::PlayAnimationOneShot: No SkeletalMeshComponent"));
		return;
	}

	UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPHelperLibrary::PlayAnimationOneShot: No AnimInstance"));
		return;
	}

	// If a montage is already playing, ignore the new request (no interrupting)
	if (AnimInst->Montage_IsPlaying(nullptr))
	{
		return;
	}

	// Stop movement during the animation if requested
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

	// Play as dynamic montage in DefaultSlot - blends in, plays once, blends out
	// Then the AnimBP state machine resumes automatically
	AnimInst->PlaySlotAnimationAsDynamicMontage(
		AnimSequence,
		FName("DefaultSlot"),
		BlendIn,
		BlendOut,
		PlayRate,
		1,      // LoopCount = 1 (play once)
		-1.0f,  // BlendOutTriggerTime = auto
		0.0f    // InTimeToStartMontageAt
	);

	// Set timer to restore movement speed after animation completes
	if (bStopMovement && MovementComp)
	{
		float Duration = AnimSequence->GetPlayLength() / FMath::Max(PlayRate, 0.01f);
		// Restore slightly before animation ends (during blend out) for smoother feel
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
			false // Don't loop
		);
	}
}
