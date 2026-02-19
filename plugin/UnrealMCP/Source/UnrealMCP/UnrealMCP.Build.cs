// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealMCP : ModuleRules
{
	public UnrealMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDefinitions.Add("UNREALMCP_EXPORTS=1");

		PublicIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(ModuleDirectory, "Public"),
				System.IO.Path.Combine(ModuleDirectory, "Public/Commands"),
				System.IO.Path.Combine(ModuleDirectory, "Public/Commands/BlueprintGraph"),
				System.IO.Path.Combine(ModuleDirectory, "Public/Commands/BlueprintGraph/Nodes")
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(ModuleDirectory, "Private"),
				System.IO.Path.Combine(ModuleDirectory, "Private/Commands"),
				System.IO.Path.Combine(ModuleDirectory, "Private/Commands/BlueprintGraph"),
				System.IO.Path.Combine(ModuleDirectory, "Private/Commands/BlueprintGraph/Nodes")
			}
		);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Networking",
				"Sockets",
				"HTTP",
				"Json",
				"JsonUtilities",
				"DeveloperSettings",
				"PhysicsCore",
				"UnrealEd",           // For Blueprint editing
				"BlueprintGraph",     // For K2Node classes (F15-F22)
				"KismetCompiler",     // For Blueprint compilation (F15-F22)
				"CinematicCamera",    // For CineCameraActor
			"EnhancedInput",      // For UInputAction, Enhanced Input events
			"AnimGraph",          // For AnimBP state machine graph nodes
			"AnimGraphRuntime",   // For FAnimNode_BlendSpacePlayer (BlendSpace1D locomotion)
			"Niagara"             // Niagara particle system spawning (F-023)
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"EditorScriptingUtilities",
				"EditorSubsystem",
				"Slate",
				"SlateCore",
				"Kismet",
				"Projects",
				"AssetRegistry",
				"MaterialEditor",        // For material editing
				"RenderCore",            // For material expressions
				"ImageWrapper",          // For PNG screenshot encoding
				"Landscape",             // For landscape/terrain editing
				"LandscapeEditor",       // For landscape editing tools
				"Foliage",               // For foliage editing
			"InputBlueprintNodes",   // For K2Node_EnhancedInputAction
				"AudioEditor",           // For USoundFactory (sound import)
				"UMG",                   // Widget blueprint creation (F-019)
				"UMGEditor",             // Widget blueprint factory (F-019)
				"AIModule",              // Behavior trees and AI controllers (F-021)
				"GameplayTasks",         // BT tasks and decorators (F-021)
				"NiagaraEditor"          // Niagara system creation from templates
			}
		);
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"PropertyEditor",      // For property editing
					"ToolMenus",           // For editor UI
					"BlueprintEditorLibrary" // For Blueprint utilities
				}
			);
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
} 