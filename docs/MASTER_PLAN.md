# it-is-unreal — Master Plan

## Overview

123-tool MCP server for controlling Unreal Engine from AI assistants. Extracted from the Blood & Dust game project as a standalone open-source tool.

## Task ID Format

| Prefix | Usage |
|--------|-------|
| `TASK-XXX` | Features and improvements |
| `BUG-XXX` | Bug fixes |
| `FEATURE-XXX` | Major features |
| `INQUIRY-XXX` | Research/investigation |

**Rules:**
- IDs are sequential (TASK-001, TASK-002...)
- Completed: ~~TASK-001~~ with strikethrough
- Never reuse IDs

## Roadmap

| ID | Title | Priority | Status | Dependencies |
|----|-------|----------|--------|--------------|
| ~~TASK-001~~ | Extract and publish open-source project | P0 | DONE | - |
| ~~TASK-002~~ | E2E testing and cross-platform audit | P1 | DONE | TASK-001 |
| ~~TASK-003~~ | Rename project to "it-is-unreal" | P2 | DONE | - |

## Completed

### ~~TASK-003~~: Rename project to "it-is-unreal"
- Renamed `server/is_it_unreal.py` → `server/it_is_unreal.py`
- Replaced 22 kebab-case, 26 snake_case, 1 PascalCase occurrences across 11 files
- Updated GitHub URLs in both .uplugin files and pyproject.toml
- Regenerated uv.lock

### ~~TASK-001~~: Extract and publish open-source project
- Scaffolded standalone project repository
- Copied 66 UnrealMCP C++ source files, applied 3 edits (BUILD_ID, MI path, DocsURL)
- Copied Python server (6431 lines) + 21 helper files, renamed imports
- Stripped GameplayHelpers (3642 -> 1215 lines), removed B&D-specific code
- Wrote README (369 lines), CLAUDE.md (41 safety rules), example MCP config
- MIT license, git init, initial commit (101 files, 42054 lines)

### ~~TASK-002~~: E2E testing and cross-platform audit
- All Python imports pass (main + 20 helpers + 123 tools)
- Fixed pyproject.toml readme path (setuptools rejected parent dir ref)
- Fixed logging FileHandler to use absolute path (Windows PermissionError)
- Fixed tool count: 123 not 124 (duplicate set_actor_property removed)
- Cross-platform audit: zero blockers, C++ uses only UE5 abstractions
- Added Win/Mac timestamp commands to CLAUDE.md rule 29
- Expanded README hot-reload note to all platforms
