#include "Commands/EpicUnrealMCPWidgetCommands.h"
#include "Commands/EpicUnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

// Widget Blueprint
#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

// Widget components
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"

// Kismet for widget creation
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

// For CreateWidget in PIE
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"

FEpicUnrealMCPWidgetCommands::FEpicUnrealMCPWidgetCommands()
{
}

TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_widget_blueprint"))
	{
		return HandleCreateWidgetBlueprint(Params);
	}
	else if (CommandType == TEXT("add_widget_to_viewport"))
	{
		return HandleAddWidgetToViewport(Params);
	}
	else if (CommandType == TEXT("set_widget_property"))
	{
		return HandleSetWidgetProperty(Params);
	}

	return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown widget command: %s"), *CommandType));
}

// ---------------------------------------------------------------------------
// create_widget_blueprint
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleCreateWidgetBlueprint(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: widget_name ---
	FString WidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'widget_name' parameter"));
	}

	// --- Optional: widget_path (default /Game/UI/) ---
	FString WidgetPath;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		WidgetPath = TEXT("/Game/UI");
	}
	// Normalize trailing slash
	if (WidgetPath.EndsWith(TEXT("/")))
	{
		WidgetPath.LeftChopInline(1);
	}

	FString FullPackagePath = WidgetPath / WidgetName;

	// --- Check if asset already exists ---
	if (UEditorAssetLibrary::DoesAssetExist(FullPackagePath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget blueprint already exists at: %s"), *FullPackagePath));
	}

	// --- Create package ---
	UPackage* Package = CreatePackage(*FullPackagePath);
	if (!Package)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create package: %s"), *FullPackagePath));
	}
	Package->FullyLoad();

	// --- Create the widget blueprint via FKismetEditorUtilities ---
	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(
		UUserWidget::StaticClass(),
		Package,
		FName(*WidgetName),
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass());

	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(BP);
	if (!WBP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create widget blueprint"));
	}

	// --- Ensure WidgetTree exists ---
	if (!WBP->WidgetTree)
	{
		WBP->WidgetTree = NewObject<UWidgetTree>(WBP, TEXT("WidgetTree"));
	}

	// --- Create root canvas panel ---
	UCanvasPanel* Canvas = WBP->WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), FName(TEXT("RootCanvas")));
	if (!Canvas)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create root CanvasPanel"));
	}
	WBP->WidgetTree->RootWidget = Canvas;

	// --- Process optional elements array ---
	int32 ElementCount = 0;
	const TArray<TSharedPtr<FJsonValue>>* ElementsArray = nullptr;
	if (Params->TryGetArrayField(TEXT("elements"), ElementsArray) && ElementsArray)
	{
		for (const TSharedPtr<FJsonValue>& ElemValue : *ElementsArray)
		{
			const TSharedPtr<FJsonObject>* ElemObjPtr = nullptr;
			if (!ElemValue->TryGetObject(ElemObjPtr) || !ElemObjPtr || !(*ElemObjPtr).IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("create_widget_blueprint: Skipping invalid element at index %d"), ElementCount);
				continue;
			}
			const TSharedPtr<FJsonObject>& ElemObj = *ElemObjPtr;

			FString ElemType;
			if (!ElemObj->TryGetStringField(TEXT("type"), ElemType))
			{
				UE_LOG(LogTemp, Warning, TEXT("create_widget_blueprint: Element missing 'type', skipping"));
				continue;
			}

			FString ElemName;
			if (!ElemObj->TryGetStringField(TEXT("name"), ElemName))
			{
				ElemName = FString::Printf(TEXT("%s_%d"), *ElemType, ElementCount);
			}

			// --- Parse position / size ---
			FVector2D Position(0.0f, 0.0f);
			FVector2D Size(100.0f, 100.0f);

			const TArray<TSharedPtr<FJsonValue>>* PosArray = nullptr;
			if (ElemObj->TryGetArrayField(TEXT("position"), PosArray) && PosArray && PosArray->Num() >= 2)
			{
				Position.X = static_cast<float>((*PosArray)[0]->AsNumber());
				Position.Y = static_cast<float>((*PosArray)[1]->AsNumber());
			}

			const TArray<TSharedPtr<FJsonValue>>* SizeArray = nullptr;
			if (ElemObj->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray && SizeArray->Num() >= 2)
			{
				Size.X = static_cast<float>((*SizeArray)[0]->AsNumber());
				Size.Y = static_cast<float>((*SizeArray)[1]->AsNumber());
			}

			// --- Construct widget by type ---
			UWidget* NewWidget = nullptr;

			if (ElemType == TEXT("ProgressBar"))
			{
				UProgressBar* PB = WBP->WidgetTree->ConstructWidget<UProgressBar>(
					UProgressBar::StaticClass(), FName(*ElemName));
				if (PB)
				{
					// Apply optional properties
					const TSharedPtr<FJsonObject>* PropsPtr = nullptr;
					if (ElemObj->TryGetObjectField(TEXT("properties"), PropsPtr) && PropsPtr && (*PropsPtr).IsValid())
					{
						double PercentVal = 0.0;
						if ((*PropsPtr)->TryGetNumberField(TEXT("Percent"), PercentVal))
						{
							PB->SetPercent(static_cast<float>(PercentVal));
						}

						const TArray<TSharedPtr<FJsonValue>>* FillColorArr = nullptr;
						if ((*PropsPtr)->TryGetArrayField(TEXT("FillColor"), FillColorArr) && FillColorArr && FillColorArr->Num() >= 3)
						{
							FLinearColor FillColor(
								static_cast<float>((*FillColorArr)[0]->AsNumber()),
								static_cast<float>((*FillColorArr)[1]->AsNumber()),
								static_cast<float>((*FillColorArr)[2]->AsNumber()),
								FillColorArr->Num() >= 4 ? static_cast<float>((*FillColorArr)[3]->AsNumber()) : 1.0f);
							PB->SetFillColorAndOpacity(FillColor);
						}
					}
					NewWidget = PB;
				}
			}
			else if (ElemType == TEXT("Image"))
			{
				UImage* Img = WBP->WidgetTree->ConstructWidget<UImage>(
					UImage::StaticClass(), FName(*ElemName));
				if (Img)
				{
					const TSharedPtr<FJsonObject>* PropsPtr = nullptr;
					if (ElemObj->TryGetObjectField(TEXT("properties"), PropsPtr) && PropsPtr && (*PropsPtr).IsValid())
					{
						const TArray<TSharedPtr<FJsonValue>>* ColorArr = nullptr;
						if ((*PropsPtr)->TryGetArrayField(TEXT("ColorAndOpacity"), ColorArr) && ColorArr && ColorArr->Num() >= 3)
						{
							FLinearColor Color(
								static_cast<float>((*ColorArr)[0]->AsNumber()),
								static_cast<float>((*ColorArr)[1]->AsNumber()),
								static_cast<float>((*ColorArr)[2]->AsNumber()),
								ColorArr->Num() >= 4 ? static_cast<float>((*ColorArr)[3]->AsNumber()) : 1.0f);
							Img->SetColorAndOpacity(Color);
						}
					}
					NewWidget = Img;
				}
			}
			else if (ElemType == TEXT("TextBlock"))
			{
				UTextBlock* TB = WBP->WidgetTree->ConstructWidget<UTextBlock>(
					UTextBlock::StaticClass(), FName(*ElemName));
				if (TB)
				{
					const TSharedPtr<FJsonObject>* PropsPtr = nullptr;
					if (ElemObj->TryGetObjectField(TEXT("properties"), PropsPtr) && PropsPtr && (*PropsPtr).IsValid())
					{
						FString TextStr;
						if ((*PropsPtr)->TryGetStringField(TEXT("Text"), TextStr))
						{
							TB->SetText(FText::FromString(TextStr));
						}

						const TArray<TSharedPtr<FJsonValue>>* ColorArr = nullptr;
						if ((*PropsPtr)->TryGetArrayField(TEXT("ColorAndOpacity"), ColorArr) && ColorArr && ColorArr->Num() >= 3)
						{
							FLinearColor Color(
								static_cast<float>((*ColorArr)[0]->AsNumber()),
								static_cast<float>((*ColorArr)[1]->AsNumber()),
								static_cast<float>((*ColorArr)[2]->AsNumber()),
								ColorArr->Num() >= 4 ? static_cast<float>((*ColorArr)[3]->AsNumber()) : 1.0f);
							TB->SetColorAndOpacity(FSlateColor(Color));
						}

						double FontSizeVal = 0.0;
						if ((*PropsPtr)->TryGetNumberField(TEXT("FontSize"), FontSizeVal))
						{
							FSlateFontInfo FontInfo = TB->GetFont();
							FontInfo.Size = static_cast<int32>(FontSizeVal);
							TB->SetFont(FontInfo);
						}
					}
					NewWidget = TB;
				}
			}
			else if (ElemType == TEXT("Border"))
			{
				UBorder* Brd = WBP->WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(), FName(*ElemName));
				if (Brd)
				{
					const TSharedPtr<FJsonObject>* PropsPtr = nullptr;
					if (ElemObj->TryGetObjectField(TEXT("properties"), PropsPtr) && PropsPtr && (*PropsPtr).IsValid())
					{
						const TArray<TSharedPtr<FJsonValue>>* ColorArr = nullptr;
						if ((*PropsPtr)->TryGetArrayField(TEXT("BrushColor"), ColorArr) && ColorArr && ColorArr->Num() >= 3)
						{
							FLinearColor Color(
								static_cast<float>((*ColorArr)[0]->AsNumber()),
								static_cast<float>((*ColorArr)[1]->AsNumber()),
								static_cast<float>((*ColorArr)[2]->AsNumber()),
								ColorArr->Num() >= 4 ? static_cast<float>((*ColorArr)[3]->AsNumber()) : 1.0f);
							Brd->SetBrushColor(Color);
						}
					}
					NewWidget = Brd;
				}
			}
			else if (ElemType == TEXT("HorizontalBox"))
			{
				UHorizontalBox* HBox = WBP->WidgetTree->ConstructWidget<UHorizontalBox>(
					UHorizontalBox::StaticClass(), FName(*ElemName));
				NewWidget = HBox;
			}
			else if (ElemType == TEXT("VerticalBox"))
			{
				UVerticalBox* VBox = WBP->WidgetTree->ConstructWidget<UVerticalBox>(
					UVerticalBox::StaticClass(), FName(*ElemName));
				NewWidget = VBox;
			}
			else if (ElemType == TEXT("Overlay"))
			{
				UOverlay* Ovl = WBP->WidgetTree->ConstructWidget<UOverlay>(
					UOverlay::StaticClass(), FName(*ElemName));
				NewWidget = Ovl;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("create_widget_blueprint: Unknown element type '%s', skipping"), *ElemType);
				continue;
			}

			if (!NewWidget)
			{
				UE_LOG(LogTemp, Warning, TEXT("create_widget_blueprint: Failed to construct widget '%s' of type '%s'"), *ElemName, *ElemType);
				continue;
			}

			// --- Add to canvas and configure slot ---
			Canvas->AddChild(NewWidget);

			UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(NewWidget->Slot);
			if (Slot)
			{
				Slot->SetPosition(Position);
				Slot->SetSize(Size);
			}

			ElementCount++;
		}
	}

	// --- Compile blueprint ---
	FKismetEditorUtilities::CompileBlueprint(WBP);

	// --- Register with asset registry ---
	FAssetRegistryModule::AssetCreated(WBP);
	Package->MarkPackageDirty();

	// --- Save package to disk ---
	FString PackageFilename = FPackageName::LongPackageNameToFilename(
		FullPackagePath, FPackageName::GetAssetPackageExtension());

	FString PackageDirectory = FPaths::GetPath(PackageFilename);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*PackageDirectory))
	{
		PlatformFile.CreateDirectoryTree(*PackageDirectory);
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, WBP, *PackageFilename, SaveArgs);

	if (!bSaved)
	{
		UE_LOG(LogTemp, Warning, TEXT("create_widget_blueprint: Failed to save package to disk: %s"), *PackageFilename);
	}

	// --- Build response ---
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("widget_name"), WidgetName);
	Data->SetStringField(TEXT("widget_path"), FullPackagePath);
	Data->SetNumberField(TEXT("element_count"), ElementCount);
	Data->SetBoolField(TEXT("saved"), bSaved);

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// add_widget_to_viewport
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleAddWidgetToViewport(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: widget_path ---
	FString WidgetPath;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'widget_path' parameter"));
	}

	// --- Optional: z_order ---
	int32 ZOrder = 0;
	double ZOrderDouble = 0.0;
	if (Params->TryGetNumberField(TEXT("z_order"), ZOrderDouble))
	{
		ZOrder = static_cast<int32>(ZOrderDouble);
	}

	// --- Load the widget blueprint ---
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(WidgetPath);
	if (!IsValid(LoadedAsset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load widget blueprint at: %s"), *WidgetPath));
	}

	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(LoadedAsset);
	if (!WBP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not a WidgetBlueprint: %s"), *WidgetPath));
	}

	// --- Check if PIE is active ---
	bool bIsPIE = GEditor && GEditor->IsPlayingSessionInEditor();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("widget_path"), WidgetPath);
	Data->SetNumberField(TEXT("z_order"), ZOrder);
	Data->SetBoolField(TEXT("is_pie_active"), bIsPIE);

	if (bIsPIE)
	{
		// --- Runtime: create widget and add to viewport ---
		UWorld* PIEWorld = nullptr;

		// Find the PIE world
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World() != nullptr)
			{
				PIEWorld = Context.World();
				break;
			}
		}

		if (!PIEWorld)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("PIE is active but no PIE world found"));
		}

		APlayerController* PC = PIEWorld->GetFirstPlayerController();
		if (!IsValid(PC))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No PlayerController found in PIE world"));
		}

		UClass* WidgetClass = WBP->GeneratedClass;
		if (!WidgetClass)
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint has no GeneratedClass (compile it first)"));
		}

		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass);
		if (!IsValid(Widget))
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create widget instance"));
		}

		Widget->AddToViewport(ZOrder);

		Data->SetStringField(TEXT("status"), TEXT("Widget added to viewport in PIE"));
	}
	else
	{
		// --- Design-time: validate only, return guidance ---
		Data->SetStringField(TEXT("status"),
			TEXT("Widget blueprint validated. To display at runtime, add a 'Create Widget' + "
			     "'Add to Viewport' node in your character/game mode Blueprint's BeginPlay event."));
	}

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ---------------------------------------------------------------------------
// set_widget_property
// ---------------------------------------------------------------------------
TSharedPtr<FJsonObject> FEpicUnrealMCPWidgetCommands::HandleSetWidgetProperty(const TSharedPtr<FJsonObject>& Params)
{
	// --- Required: widget_path ---
	FString WidgetPath;
	if (!Params->TryGetStringField(TEXT("widget_path"), WidgetPath))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'widget_path' parameter"));
	}

	// --- Required: widget_name (child widget inside the blueprint) ---
	FString ChildWidgetName;
	if (!Params->TryGetStringField(TEXT("widget_name"), ChildWidgetName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'widget_name' parameter"));
	}

	// --- Required: property_name ---
	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'property_name' parameter"));
	}

	// --- Required: value ---
	if (!Params->HasField(TEXT("value")))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required 'value' parameter"));
	}
	TSharedPtr<FJsonValue> JsonValue = Params->TryGetField(TEXT("value"));

	// --- Load widget blueprint ---
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(WidgetPath);
	if (!IsValid(LoadedAsset))
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to load widget blueprint at: %s"), *WidgetPath));
	}

	UWidgetBlueprint* WBP = Cast<UWidgetBlueprint>(LoadedAsset);
	if (!WBP)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset is not a WidgetBlueprint: %s"), *WidgetPath));
	}

	if (!WBP->WidgetTree)
	{
		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Widget blueprint has no WidgetTree"));
	}

	// --- Find the named child widget ---
	UWidget* TargetWidget = WBP->WidgetTree->FindWidget(FName(*ChildWidgetName));
	if (!TargetWidget)
	{
		// Gather available widget names for error message
		TArray<FString> AvailableNames;
		WBP->WidgetTree->ForEachWidget([&AvailableNames](UWidget* W)
		{
			if (W)
			{
				AvailableNames.Add(W->GetName());
			}
		});

		return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Widget '%s' not found in WidgetTree. Available: [%s]"),
				*ChildWidgetName, *FString::Join(AvailableNames, TEXT(", "))));
	}

	// --- Apply property based on known widget types ---
	bool bPropertySet = false;
	FString ResultValue;

	// --- UProgressBar ---
	UProgressBar* PB = Cast<UProgressBar>(TargetWidget);
	if (PB)
	{
		if (PropertyName == TEXT("Percent"))
		{
			float Val = static_cast<float>(JsonValue->AsNumber());
			PB->SetPercent(Val);
			bPropertySet = true;
			ResultValue = FString::SanitizeFloat(Val);
		}
		else if (PropertyName == TEXT("FillColor") || PropertyName == TEXT("FillColorAndOpacity"))
		{
			const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
			if (JsonValue->TryGetArray(Arr) && Arr && Arr->Num() >= 3)
			{
				FLinearColor Color(
					static_cast<float>((*Arr)[0]->AsNumber()),
					static_cast<float>((*Arr)[1]->AsNumber()),
					static_cast<float>((*Arr)[2]->AsNumber()),
					Arr->Num() >= 4 ? static_cast<float>((*Arr)[3]->AsNumber()) : 1.0f);
				PB->SetFillColorAndOpacity(Color);
				bPropertySet = true;
				ResultValue = Color.ToString();
			}
		}
	}

	// --- UTextBlock ---
	UTextBlock* TB = Cast<UTextBlock>(TargetWidget);
	if (TB)
	{
		if (PropertyName == TEXT("Text"))
		{
			FString TextStr = JsonValue->AsString();
			TB->SetText(FText::FromString(TextStr));
			bPropertySet = true;
			ResultValue = TextStr;
		}
		else if (PropertyName == TEXT("ColorAndOpacity"))
		{
			const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
			if (JsonValue->TryGetArray(Arr) && Arr && Arr->Num() >= 3)
			{
				FLinearColor Color(
					static_cast<float>((*Arr)[0]->AsNumber()),
					static_cast<float>((*Arr)[1]->AsNumber()),
					static_cast<float>((*Arr)[2]->AsNumber()),
					Arr->Num() >= 4 ? static_cast<float>((*Arr)[3]->AsNumber()) : 1.0f);
				TB->SetColorAndOpacity(FSlateColor(Color));
				bPropertySet = true;
				ResultValue = Color.ToString();
			}
		}
		else if (PropertyName == TEXT("FontSize"))
		{
			int32 FontSize = static_cast<int32>(JsonValue->AsNumber());
			FSlateFontInfo FontInfo = TB->GetFont();
			FontInfo.Size = FontSize;
			TB->SetFont(FontInfo);
			bPropertySet = true;
			ResultValue = FString::FromInt(FontSize);
		}
	}

	// --- UImage ---
	UImage* Img = Cast<UImage>(TargetWidget);
	if (Img)
	{
		if (PropertyName == TEXT("ColorAndOpacity"))
		{
			const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
			if (JsonValue->TryGetArray(Arr) && Arr && Arr->Num() >= 3)
			{
				FLinearColor Color(
					static_cast<float>((*Arr)[0]->AsNumber()),
					static_cast<float>((*Arr)[1]->AsNumber()),
					static_cast<float>((*Arr)[2]->AsNumber()),
					Arr->Num() >= 4 ? static_cast<float>((*Arr)[3]->AsNumber()) : 1.0f);
				Img->SetColorAndOpacity(Color);
				bPropertySet = true;
				ResultValue = Color.ToString();
			}
		}
		else if (PropertyName == TEXT("Visibility"))
		{
			FString VisStr = JsonValue->AsString();
			ESlateVisibility Vis = ESlateVisibility::Visible;
			if (VisStr == TEXT("Hidden"))
			{
				Vis = ESlateVisibility::Hidden;
			}
			else if (VisStr == TEXT("Collapsed"))
			{
				Vis = ESlateVisibility::Collapsed;
			}
			else if (VisStr == TEXT("HitTestInvisible"))
			{
				Vis = ESlateVisibility::HitTestInvisible;
			}
			else if (VisStr == TEXT("SelfHitTestInvisible"))
			{
				Vis = ESlateVisibility::SelfHitTestInvisible;
			}
			Img->SetVisibility(Vis);
			bPropertySet = true;
			ResultValue = VisStr;
		}
	}

	// --- Fallback: use reflection-based property setter ---
	if (!bPropertySet)
	{
		FString ErrorMsg;
		bPropertySet = FEpicUnrealMCPCommonUtils::SetObjectProperty(TargetWidget, PropertyName, JsonValue, ErrorMsg);
		if (bPropertySet)
		{
			ResultValue = TEXT("(set via reflection)");
		}
		else
		{
			return FEpicUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Failed to set property '%s' on widget '%s' (%s): %s"),
					*PropertyName, *ChildWidgetName, *TargetWidget->GetClass()->GetName(), *ErrorMsg));
		}
	}

	// --- Mark dirty ---
	WBP->GetPackage()->MarkPackageDirty();

	// --- Build response ---
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("widget_path"), WidgetPath);
	Data->SetStringField(TEXT("widget_name"), ChildWidgetName);
	Data->SetStringField(TEXT("property_name"), PropertyName);
	Data->SetStringField(TEXT("value"), ResultValue);

	return FEpicUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}
