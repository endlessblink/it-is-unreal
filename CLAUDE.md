# CLAUDE.md — AI Safety Rules for UnrealMCP

These 41 rules were learned the hard way through months of real-world Unreal Engine + MCP development. Each rule exists because violating it CRASHES Unreal Engine, corrupts data, or causes silent failures. They are non-negotiable.

## Actor Deletion Rules

1. **NEVER use `World->DestroyActor()` or `Actor->Destroy()` in editor context.** Use `EditorActorSubsystem::DestroyActors()` instead — it handles editor notifications, scene outliner updates, and OFPA package cleanup.

2. **NEVER call `Actor->Rename()` on editor actors.** UE5+ OFPA (One File Per Actor) causes `UDeletedObjectPlaceholder` assertion crashes when renaming actors that have external packages.

3. **NEVER delete and spawn actors with the same name in the same tick.** `DestroyActor` marks pending-kill but the name stays registered in `FUObjectHashTables` until GC runs. Spawning with the same name causes a "Cannot generate unique name" fatal crash.

4. **NEVER destroy actors during iteration.** Collect actors into a TArray first, then destroy from the array.

## Actor Spawning Rules

5. **ALWAYS use `ESpawnActorNameMode::Requested`** (not the default `Required_Fatal`). This gracefully generates a unique name if the requested name is taken, instead of crashing.

6. **NEVER reuse actor names.** Always append a unique suffix (timestamp, GUID, counter). Names are only freed after garbage collection, not after DestroyActor.

7. **ALWAYS check `IsValid(Actor)` before operating on any actor pointer.** `IsPendingKill()` is deprecated in UE5.

## Bulk Operation Rules

8. **NEVER perform more than 3 spawn/destroy operations in a single FTSTicker tick.** Each spawn/destroy triggers component registration, physics scene updates, and rendering cleanup. More than 3 per tick risks crashes and frame hitches.

9. **NEVER call MCP tools in parallel.** Each MCP tool creates its own FTSTicker callback. Multiple callbacks firing in the same frame can crash the engine.

10. **Separate delete and spawn phases.** If replacing actors: Phase 1 = destroy all (spread across ticks), then Phase 2 = spawn new (spread across ticks). Never interleave.

## Viewport & Screenshot Rules

11. **NEVER call `FlushRenderingCommands()` in FTSTicker callbacks.** It blocks the game thread waiting for the render thread, risking deadlocks. Let the engine handle rendering sync naturally.

12. **Use `FScreenshotRequest::RequestScreenshot()` for screenshots** instead of direct `Viewport->ReadPixels()`. The request API handles rendering synchronization internally. On Linux, `GetSizeXY()` returns (0,0) even when the editor is running — direct ReadPixels does not work.

13. **Use `GEditor->GetLevelViewportClients()` for viewport access**, not `GetAllViewportClients()` or `GetActiveViewport()`, which may return non-level viewports (material editors, asset previews).

## Texture Import & Heavy Asset Rules

14. **NEVER import more than 2 textures without pausing.** Each `import_texture` call triggers `PostEditChange()` (texture recompression), `UpdateResource()` (GPU upload), and `AssetCreated()` (registry broadcast). Back-to-back imports create catastrophic memory pressure and notification storms that can crash the editor or corrupt landscape streaming proxies.

15. **Treat `import_texture` as a HEAVY operation** on par with `import_mesh`. Each import decompresses the source image (~64MB for a 4K texture), builds platform mip chains, and broadcasts to all editor subsystems synchronously.

16. **After every 2 texture imports, call a lightweight operation** (e.g., `get_actors_in_level`, `list_assets`, or `does_asset_exist`) to give the engine a full tick to process streaming updates, GC, and notification queues before importing more textures.

17. **NEVER import textures while landscape operations are pending.** Texture imports trigger AssetRegistry notifications that can cause World Partition to re-evaluate landscape streaming proxy loading, potentially unloading or corrupting landscape sections.

18. **`CreatePackage()` + `MarkPackageDirty()` without save accumulates in memory.** Each unsaved package stays in RAM until the user saves. Multiple unsaved texture packages compete with landscape streaming proxies for memory budget.

## Material Creation Rules

19. **UE 5.7 has NO `bUsedWithLandscape` flag.** Landscape materials compile without special usage flags. Checkerboard on landscape = shader compilation error — check for ERROR nodes in the material graph.

20. **ALWAYS match vector dimensions in material expressions.** Texture samples output float4 (RGBA). UV coordinates are float2. Before combining them (Add, Multiply), use ComponentMask to match dimensions. `Add(float2, float4)` = ERROR node.

21. **ALWAYS verify shader compilation after creating or modifying materials.** Check for checkerboard in the viewport or ERROR labels on nodes. MCP returns "success" even when connections are invalid.

22. **NEVER use LandscapeLayerCoords for UV generation** — `mapping_scale` does NOT persist through editor restart. Use `WorldPosition * Constant` instead (values always persist).

23. **NEVER use LandscapeLayerBlend via MCP** — causes `TextureReferenceIndex != INDEX_NONE` crash in HLSLMaterialTranslator.cpp.

24. **NEVER build more than 10 material nodes without recompiling and visually verifying.** Material graph errors are silent — MCP returns "success" even when connections are invalid. Verify early and often.

## Material Best Practices

25. **NEVER connect AmbientOcclusion from ARM textures.** ARM textures from Megascans/PolyHaven have AO=0 in UV padding areas, causing dark glossy patches on meshes. UE5 Lumen handles ambient occlusion automatically.

## Anti-Tiling Rules

26. **NEVER use texture bombing for anti-tiling** (sampling the same texture twice at fixed UV offsets and blending with noise). The noise frequency inevitably matches the tile frequency, so each noise blob covers exactly one tile — the grid is still visible. Use **UV noise distortion** instead: warp UV coordinates with continuous low-frequency noise BEFORE sampling, so every pixel gets a unique UV offset.

27. **UV distortion warp_scale must be ~80x smaller than detail_uv_scale.** If `detail_uv_scale=0.004`, `warp_scale` should be ~0.00005. If they are too close, the warp pattern itself becomes visible. If too far apart, the warp has no visible effect on the tile grid.

28. **Always use two separate noise nodes for X and Y UV warp.** Feed the Y noise node a position-offset version of WorldPos (e.g., `WorldPos + Constant3Vector(1000, 2000, 0)`) to get a decorrelated pattern. Using the same noise for both axes creates directional smearing artifacts.

## Binary vs Source Freshness

29. **ALWAYS check if the compiled `.so` / `.dll` binary is newer than ALL source files BEFORE debugging runtime issues.** Linux UE5 does NOT hot-reload `.so` files. If the binary is older than any source file, the editor is running stale code. Stop investigating, rebuild the plugin, and restart the editor.

    Check timestamps: `stat -c %Y Plugins/*/Binaries/Linux/*.so` vs source `.cpp`/`.h` files.

## General MCP Rules

30. **ALWAYS call MCP tools strictly sequentially.** Wait for one response before sending the next command.

31. **NEVER batch heavy operations in a single FTSTicker callback.** If an operation needs to process N items, use a recurring ticker that processes 3 items per tick.

32. **Test new C++ changes with a SINGLE lightweight operation first** before attempting bulk operations. Verify the fix works before scaling up.

33. **Heavy operations require extra caution.** The following MCP commands are HEAVY: `import_texture`, `import_mesh`, `import_skeletal_mesh`, `create_pbr_material`, `create_landscape_material`, `scatter_meshes_on_landscape`. Always allow a full engine tick between consecutive heavy operations.

34. **FTSTicker callbacks can stack.** If commands arrive faster than the game thread processes them, multiple ticker callbacks queue up and fire back-to-back in the same or consecutive frames with no breathing room for engine subsystems (streaming, GC, landscape component updates).

35. **`PostEditChange()` is a nuclear broadcast.** It triggers dozens of subsystem notifications synchronously (texture streaming, shader recompilation, asset registry updates). Calling it 6+ times in rapid succession overwhelms the editor's ability to maintain consistency.

## Skeletal Mesh & Animation Rules

36. **Skeletal mesh import is HEAVIER than static mesh.** It creates a USkeleton, UPhysicsAsset, and processes skin weights and bone hierarchy. Allow 5+ seconds cooldown before the next heavy operation.

37. **Animation-only import REQUIRES an existing skeleton.** Always validate that `skeleton_path` resolves to a valid `USkeleton` before calling `import_animation`. Null skeleton = crash or silent failure.

38. **ACharacter CDO components are NOT in SimpleConstructionScript.** `UCapsuleComponent`, `USkeletalMeshComponent` (Mesh), and `UCharacterMovementComponent` are created in the C++ constructor. To modify their defaults in a Blueprint: compile the BP first, then access via `Blueprint->GeneratedClass->GetDefaultObject<ACharacter>()`.

39. **`UAnimBlueprintFactory` CRASHES if `TargetSkeleton` is null** and `bTemplate` is false. Always validate the skeleton asset exists and loads successfully before calling `FactoryCreateNew()`.

40. **FBX files can produce multiple assets.** When importing a skeletal mesh with `bImportAnimations=true`, iterate `ImportTask->GetObjects()` to find all created assets (SkeletalMesh + AnimSequence[]). Do not assume a single result.

41. **NEVER import skeletal mesh + animation in parallel MCP calls.** Always strictly sequential with breathing room between each. The skeleton created by mesh import must be fully saved before animation import references it.
