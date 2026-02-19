#include "LandscapeParameterController.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Components/SceneComponent.h"
#include "UObject/ConstructorHelpers.h"

ALandscapeParameterController::ALandscapeParameterController()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Set via editor property or Blueprint
	MaterialInstancePath.SetPath(TEXT(""));
}

#if WITH_EDITOR
void ALandscapeParameterController::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SyncParametersToMI();
}
#endif

void ALandscapeParameterController::SyncParametersToMI()
{
	UObject* LoadedObj = MaterialInstancePath.TryLoad();
	UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(LoadedObj);
	if (!MI)
	{
		UE_LOG(LogTemp, Warning, TEXT("LandscapeParameterController: Could not load MI at '%s'"), *MaterialInstancePath.ToString());
		return;
	}

	// Helper lambda to set a scalar parameter on the MI
	auto SetParam = [MI](const FName& ParamName, float Value)
	{
		// Check if parameter exists in the MI's parent material
		FMaterialParameterInfo ParamInfo(ParamName);
		float CurrentValue = 0.0f;
		if (MI->GetScalarParameterValue(ParamInfo, CurrentValue))
		{
			// Only update if value actually changed
			if (!FMath::IsNearlyEqual(CurrentValue, Value, 0.0001f))
			{
				MI->SetScalarParameterValueEditorOnly(ParamInfo, Value);
			}
		}
		else
		{
			// Parameter doesn't exist yet in overrides, set it
			MI->SetScalarParameterValueEditorOnly(ParamInfo, Value);
		}
	};

	// Terrain Blending
	SetParam(FName(TEXT("SlopeSharpness")), SlopeSharpness);
	SetParam(FName(TEXT("GrassAmount")), GrassAmount);
	SetParam(FName(TEXT("RubbleAmount")), RubbleAmount);
	SetParam(FName(TEXT("PebbleAmount")), PebbleAmount);

	// Surface Detail
	SetParam(FName(TEXT("DirtAmount")), DirtAmount);
	SetParam(FName(TEXT("DirtSize")), DirtSize);
	SetParam(FName(TEXT("PuddleAmount")), PuddleAmount);
	SetParam(FName(TEXT("PuddleSize")), PuddleSize);

	// Material Properties
	SetParam(FName(TEXT("NormalStrength")), NormalStrength);
	SetParam(FName(TEXT("Roughness")), Roughness);
	SetParam(FName(TEXT("Metallic")), Metallic);

	MI->MarkPackageDirty();

	// Trigger material update so viewport reflects changes immediately
	MI->PostEditChange();
}
