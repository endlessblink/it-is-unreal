// C++ AnimInstance for enemy locomotion via BlendSpace1D.
// Provides smoothed Speed variable for continuous idle/walk blending.
// Eliminates state-machine animation resets.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;

UCLASS()
class GAMEPLAYHELPERS_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Native cached speed used by C++ update logic. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float AnimSpeed = 0.0f;

	/** True when the enemy is dead — freezes the current pose. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	TWeakObjectPtr<ACharacter> CachedCharacter;
	TWeakObjectPtr<UCharacterMovementComponent> CachedCMC;
};
