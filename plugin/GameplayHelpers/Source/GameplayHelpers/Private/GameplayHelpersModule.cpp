#include "Modules/ModuleManager.h"

class FGameplayHelpersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UE_LOG(LogTemp, Display, TEXT("=== GameplayHelpers v1.0.0 LOADED ==="));
	}

	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGameplayHelpersModule, GameplayHelpers)
