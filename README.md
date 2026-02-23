<p align="center">
  <img src="docs/it-is-unreal-cover.png" alt="it-is-unreal" width="100%">
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="MIT License"></a>
  <img src="https://img.shields.io/badge/Unreal_Engine-5.4%2B-black?logo=unrealengine" alt="Unreal Engine 5.4+">
  <img src="https://img.shields.io/badge/Python-3.10%2B-3776AB?logo=python&logoColor=white" alt="Python 3.10+">
  <img src="https://img.shields.io/badge/MCP_Tools-123-green" alt="123 MCP Tools">
</p>

# it-is-unreal

**123-tool MCP server for controlling Unreal Engine from AI assistants.**

> Control every aspect of Unreal Engine — actors, materials, blueprints, landscapes, animations, AI, and more — directly from Claude, ChatGPT, or any MCP-compatible AI assistant.

## Architecture

```
AI Assistant <--stdio--> Python MCP Server <--TCP:55557--> UE5 C++ Plugin
```

The system has two components:

1. **C++ Editor Plugin** (`UnrealMCP`) — Runs inside Unreal Editor, listens on TCP port 55557, executes commands on the game thread via `FTSTicker`
2. **Python MCP Server** (`it-is-unreal`) — Bridges MCP protocol (stdio) to Unreal Engine (TCP), exposes 123 tools to AI assistants

An optional companion plugin (`GameplayHelpers`) provides runtime Blueprint helper functions for character input, animation, combat, and enemy AI — useful for rapid prototyping via AI.

## Quick Start

### Prerequisites

- **Unreal Engine** 5.4+ (tested with 5.7)
- **Python** 3.10+
- **uv** — install from [docs.astral.sh/uv/getting-started/installation](https://docs.astral.sh/uv/getting-started/installation/)

### Step 1: Clone

```bash
git clone https://github.com/endlessblink/it-is-unreal.git
```

### Step 2: Install the UE5 Plugin

Copy the plugin folders into your Unreal project's `Plugins/` directory:

```bash
# macOS / Linux
cp -r /absolute/path/to/it-is-unreal/plugin/UnrealMCP /absolute/path/to/YourProject/Plugins/

# Windows (PowerShell)
Copy-Item -Recurse C:\path\to\it-is-unreal\plugin\UnrealMCP C:\path\to\YourProject\Plugins\
```

Your project structure should look like:

```
YourProject/
  Plugins/
    UnrealMCP/          <- required
    GameplayHelpers/    <- optional, copy only if you want runtime Blueprint helpers
```

Open the project in Unreal Editor. If prompted to rebuild, click **Yes**. The plugin is editor-only and loads automatically on startup.

### Step 3: Configure Your AI Client

Add the MCP server config to your client. Replace `/absolute/path/to/it-is-unreal` with the actual absolute path where you cloned the repository — do not use a relative path or a placeholder.

Your MCP client launches the server automatically over stdio; you do not run it manually.

<details>
<summary>Claude Code</summary>

Merge into `.claude/settings.json` (project-local) or `~/.claude/settings.json` (global):

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": ["--directory", "/absolute/path/to/it-is-unreal/server", "run", "it_is_unreal.py"]
    }
  }
}
```

</details>

<details>
<summary>Claude Desktop</summary>

Merge into `claude_desktop_config.json`:

- macOS: `~/Library/Application Support/Claude/claude_desktop_config.json`
- Windows: `%APPDATA%\Claude\claude_desktop_config.json`

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": ["--directory", "/absolute/path/to/it-is-unreal/server", "run", "it_is_unreal.py"]
    }
  }
}
```

</details>

<details>
<summary>Cursor</summary>

Add to `.cursor/mcp.json` in your project root, or `~/.cursor/mcp.json` globally:

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": ["--directory", "/absolute/path/to/it-is-unreal/server", "run", "it_is_unreal.py"]
    }
  }
}
```

</details>

<details>
<summary>pip install alternative (any client)</summary>

If you prefer a system-installed entry point:

```bash
cd /absolute/path/to/it-is-unreal/server && pip install -e .
```

Then use this config in any client:

```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "it-is-unreal"
    }
  }
}
```

</details>

### Step 4: Verify

Open your Unreal project in the editor, then ask your AI assistant:

> *List all actors in the current level*

If you get actor names back, the connection is working.

## Tools (123)

### Actor Management (6)

| Tool | Description |
|------|-------------|
| `get_actors_in_level` | List all actors in the current level |
| `find_actors_by_name` | Find actors matching a name pattern |
| `spawn_actor` | Spawn actors (StaticMesh, PointLight, Camera, etc.) |
| `delete_actor` | Delete an actor by name |
| `delete_actors_by_pattern` | Bulk delete actors matching a name pattern |
| `set_actor_transform` | Set position, rotation, and scale |

### Actor Properties & Info (4)

| Tool | Description |
|------|-------------|
| `get_actor_properties` | Get detailed properties of an actor |
| `set_actor_property` | Set any property via reflection |
| `get_actor_material_info` | Get material info for an actor |
| `snap_actor_to_ground` | Snap actor down to terrain surface |

### Materials (9)

| Tool | Description |
|------|-------------|
| `create_material` | Create a new material asset |
| `create_material_instance` | Create a material instance from a parent |
| `set_material_instance_parameter` | Set scalar, vector, or texture parameters |
| `create_pbr_material` | Create a full PBR material from texture paths |
| `apply_material_to_actor` | Apply material to an actor in the level |
| `set_mesh_asset_material` | Set default material on a mesh asset |
| `apply_material_to_blueprint` | Apply material to a Blueprint's mesh component |
| `set_mesh_material_color` | Quick color change on a mesh |
| `get_available_materials` | List all materials in the project |

### Material Graph (9)

| Tool | Description |
|------|-------------|
| `create_material_asset` | Create material with full graph access |
| `add_material_expression` | Add expression nodes to a material graph |
| `connect_material_expressions` | Wire two material graph nodes together |
| `connect_to_material_output` | Connect a node to a material output pin |
| `set_material_expression_property` | Set properties on expression nodes |
| `delete_material_expression` | Remove nodes from the material graph |
| `recompile_material` | Force recompile a material |
| `get_material_graph` | Inspect a material graph's node structure |
| `get_material_info` | Get material details and parameter list |

### Textures & Assets (8)

| Tool | Description |
|------|-------------|
| `import_texture` | Import image files as Unreal textures |
| `import_sound` | Import WAV/OGG files as sound assets |
| `set_texture_properties` | Modify texture compression and settings |
| `get_texture_info` | Get texture dimensions and format details |
| `list_assets` | Browse the project's asset folders |
| `does_asset_exist` | Check if an asset path is valid |
| `get_asset_info` | Get detailed metadata for any asset |
| `delete_asset` | Delete a project asset |

### Meshes & Import (6)

| Tool | Description |
|------|-------------|
| `import_mesh` | Import FBX/OBJ as a static mesh |
| `import_skeletal_mesh` | Import FBX as a skeletal mesh with skeleton |
| `import_animation` | Import FBX animation for an existing skeleton |
| `set_static_mesh_properties` | Configure mesh LOD and collision settings |
| `set_nanite_enabled` | Toggle Nanite virtualized geometry on meshes |
| `set_physics_properties` | Configure physics simulation on mesh assets |

### Landscape (11)

| Tool | Description |
|------|-------------|
| `create_landscape_material` | Create a landscape material with anti-tiling UV distortion |
| `create_landscape_layer` | Create a landscape layer info asset |
| `add_layer_to_landscape` | Add a paint layer to the landscape |
| `set_landscape_material` | Apply a material to the landscape |
| `sculpt_landscape` | Sculpt terrain height at a location |
| `smooth_landscape` | Smooth terrain in an area |
| `flatten_landscape` | Flatten terrain to a target height |
| `paint_landscape_layer` | Paint landscape layers |
| `configure_landscape_layer_blend` | Configure layer blend settings |
| `get_landscape_info` | Get landscape dimensions and section info |
| `get_landscape_layers` | List all paint layers on the landscape |

### Blueprints (9)

| Tool | Description |
|------|-------------|
| `create_blueprint` | Create a new Blueprint class |
| `add_component_to_blueprint` | Add components to a Blueprint |
| `compile_blueprint` | Compile with full error and warning reporting |
| `read_blueprint_content` | Read Blueprint structure and variables |
| `analyze_blueprint_graph` | Analyze Blueprint node graph connections |
| `get_blueprint_variable_details` | Get details for a Blueprint variable |
| `get_blueprint_function_details` | Get details for a Blueprint function |
| `spawn_blueprint_actor_in_level` | Spawn an instance of a Blueprint in the level |
| `spawn_physics_blueprint_actor` | Spawn a Blueprint actor with physics enabled |

### Blueprint Graph (12)

| Tool | Description |
|------|-------------|
| `add_node` | Add any node type to a Blueprint graph |
| `connect_nodes` | Wire two Blueprint nodes together |
| `delete_node` | Remove a node from a Blueprint graph |
| `set_node_property` | Set node defaults and pin default values |
| `create_variable` | Create a new Blueprint variable |
| `set_blueprint_variable_properties` | Configure variable type, defaults, and flags |
| `add_event_node` | Add event nodes (BeginPlay, Tick, custom, etc.) |
| `create_function` | Create a new Blueprint function |
| `add_function_input` | Add input parameters to a Blueprint function |
| `add_function_output` | Add output/return values to a Blueprint function |
| `delete_function` | Remove a Blueprint function |
| `rename_function` | Rename a Blueprint function |

### Characters & Animation (9)

| Tool | Description |
|------|-------------|
| `create_character_blueprint` | Create a character Blueprint class |
| `create_anim_blueprint` | Create an Animation Blueprint |
| `setup_locomotion_state_machine` | Wire a locomotion state machine in an AnimBP |
| `setup_blendspace_locomotion` | Create BlendSpace1D locomotion and wire AnimGraph |
| `set_character_properties` | Configure character movement and capsule properties |
| `auto_fit_capsule` | Auto-size capsule to imported mesh geometry bounds |
| `set_anim_sequence_root_motion` | Toggle root motion on an animation sequence |
| `set_anim_state_always_reset_on_entry` | Configure animation state reset behavior |
| `set_state_machine_max_transitions_per_frame` | Tune state machine transition performance |

### Input (3)

| Tool | Description |
|------|-------------|
| `create_input_action` | Create an Enhanced Input action asset |
| `add_input_mapping` | Map keys to input actions in a mapping context |
| `add_enhanced_input_action_event` | Wire an input action event to a Blueprint |

### Gameplay (7)

| Tool | Description |
|------|-------------|
| `set_game_mode_default_pawn` | Set the GameMode's default pawn class |
| `create_anim_montage` | Create an animation montage asset |
| `play_montage_on_actor` | Play a montage on an actor at runtime |
| `apply_impulse` | Apply a physics impulse to an actor |
| `trigger_post_process_effect` | Trigger post-process effects on a volume |
| `spawn_niagara_system` | Spawn a Niagara particle system in the level |
| `add_anim_notify` | Add notify events to an animation sequence |

### Niagara VFX (3)

| Tool | Description |
|------|-------------|
| `create_niagara_system` | Create a Niagara system from a template emitter |
| `set_niagara_parameter` | Set parameters on a Niagara component |
| `create_atmospheric_fx` | Create atmospheric effects (dust, fog, particles) |

### UI / Widgets (3)

| Tool | Description |
|------|-------------|
| `create_widget_blueprint` | Create a UMG widget Blueprint |
| `add_widget_to_viewport` | Add a widget Blueprint to the player's screen |
| `set_widget_property` | Configure widget properties |

### AI / Behavior Trees (5)

| Tool | Description |
|------|-------------|
| `create_behavior_tree` | Create a behavior tree asset |
| `create_blackboard` | Create a blackboard asset |
| `add_bt_task` | Add a task node to a behavior tree |
| `add_bt_decorator` | Add a decorator to a behavior tree node |
| `assign_behavior_tree` | Assign a behavior tree to an AI controller |

### World Building (6)

| Tool | Description |
|------|-------------|
| `scatter_meshes_on_landscape` | Scatter static mesh props on landscape with line traces |
| `scatter_foliage` | HISM-based vegetation scatter with Poisson disk distribution |
| `get_height_at_location` | Query terrain height at a world location |
| `focus_viewport_on_actor` | Focus the editor camera on an actor |
| `take_screenshot` | Capture a screenshot of the editor viewport |
| `get_editor_log` | Read recent editor log messages |

### Skeletal Mesh Extras (1)

| Tool | Description |
|------|-------------|
| `set_skeletal_animation` | Set the animation on a skeletal mesh component |

### Procedural Generation (12)

| Tool | Description |
|------|-------------|
| `create_pyramid` | Generate pyramid geometry from primitives |
| `create_wall` | Generate modular wall structures |
| `create_tower` | Generate tower structures |
| `create_staircase` | Generate staircases |
| `create_arch` | Generate arch structures |
| `construct_house` | Generate a complete house from components |
| `construct_mansion` | Generate a multi-wing mansion |
| `create_maze` | Generate a maze layout |
| `create_town` | Generate an entire town with streets and buildings |
| `create_castle_fortress` | Generate a castle complex with walls and towers |
| `create_suspension_bridge` | Generate a suspension bridge |
| `create_aqueduct` | Generate a Roman-style aqueduct |

## Platform Support

| Platform | Status |
|----------|--------|
| Windows (Win64) | Supported |
| macOS (Mac) | Supported |
| Linux | Supported |

## GameplayHelpers Plugin (Optional)

The `GameplayHelpers` plugin provides runtime Blueprint-callable functions for rapid prototyping. Unlike the editor plugin, these functions work in packaged builds.

| Function | Description |
|----------|-------------|
| `SetCharacterWalkSpeed` | Set character max walk speed at runtime |
| `PlayAnimationOneShot` | Play an animation as a one-shot dynamic montage with blend in/out |
| `AddInputMappingContextToCharacter` | Wire Enhanced Input mapping context from BeginPlay |
| `ApplyMeleeDamage` | Sphere overlap + cone filter melee damage with knockback |
| `UpdateEnemyAI` | Full tick-based enemy AI (Idle/Chase/Attack/HitReact/Death/Patrol/Return) |
| `SetPlayerBlocking` | Enable blocking stance (75% damage reduction) |
| `IsPlayerBlocking` | Check if the player is currently blocking |

The `UpdateEnemyAI` function supports personality archetypes (Normal, Berserker, Stalker, Brute, Crawler), per-instance randomization, combat partners, patrol behavior, idle behaviors, and multi-animation combat. Call it from Event Tick on each enemy Blueprint — state is managed internally per-actor.

## How It Works

The Python MCP server receives tool calls from your AI assistant over stdio, serializes them to JSON, and sends them to the Unreal Editor plugin over a persistent TCP connection on port 55557. The plugin receives the command, queues execution via `FTSTicker` to run on the game thread, executes the corresponding C++ handler, and returns a JSON result.

Heavy operations (texture import, mesh import, procedural generation) use longer timeouts (up to 300 seconds) and enforce post-execution cooldowns to prevent the engine from being overwhelmed with concurrent subsystem notifications.

## Connection Details

- **Host**: `127.0.0.1` (localhost only)
- **Port**: `55557`
- **Protocol**: JSON over TCP, length-prefixed
- **Retry**: Exponential backoff, 3 attempts, 0.5s–5.0s delay

The server connects fresh for each tool call and disconnects after receiving the response. There is no persistent session state between calls.

## Troubleshooting

**Plugin not loading:**
Check the Output Log in Unreal Editor (Window > Developer Tools > Output Log) and filter for `LogMCP`. You should see `UnrealMCP server listening on port 55557` on startup. If not, verify the plugin is in `YourProject/Plugins/UnrealMCP/` and rebuild.

**Connection refused:**
The Unreal Editor must be running with the plugin loaded before your AI assistant can connect. The Python server connects to `127.0.0.1:55557` by default — override with `UNREAL_HOST` and `UNREAL_PORT` environment variables if needed.

**Tool call timeouts:**
Heavy operations (texture import, mesh import, procedural generation) can take up to 300 seconds. If you hit timeouts on normal operations, check that no dialog boxes are blocking the editor (e.g., "Rebuild?" prompts).

**Server logs:**
The Python server writes detailed logs to `server/it_is_unreal.log`. Check this file for connection errors, malformed responses, and tool execution details.

## Contributing

Contributions welcome. Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test against a live Unreal Editor instance
5. Submit a pull request

When adding new tools, follow the existing pattern: define the C++ handler in the plugin, register it in the appropriate handler class, and add the corresponding `@mcp.tool()` function in `it_is_unreal.py`.

## Updating

```bash
cd it-is-unreal && git pull
```

Then re-copy `plugin/UnrealMCP/` to your Unreal project's `Plugins/` directory and restart the editor. The Python server picks up changes automatically on next launch (your MCP client restarts it per tool call).

## Safety Rules

See [CLAUDE.md](CLAUDE.md) for 41 hard-won safety rules for working with Unreal Engine via MCP. These prevent crashes, data loss, and silent failures discovered through extensive real-world development.
CLAUDE.md is automatically read by AI coding assistants (Claude Code, Cursor, etc.) so these rules are enforced whenever an AI works on your Unreal project.

Key rules at a glance:

- Never call MCP tools in parallel — each creates an FTSTicker callback, and simultaneous callbacks crash the engine
- Never import more than 2 textures before a lightweight pause — each 4K texture is ~64MB of RAM
- Never use `World->DestroyActor()` in editor context — use `EditorActorSubsystem::DestroyActors()` instead
- UE5 does NOT hot-reload plugin binaries (`.so`/`.dll`/`.dylib`) — always rebuild and restart the editor after C++ changes

## License

[MIT](LICENSE)

---

Built with frustration, determination, and mass amounts of 3AM coffee during the development of Blood & Dust.
