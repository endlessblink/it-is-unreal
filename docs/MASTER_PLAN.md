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
| ~~TASK-004~~ | Prepare project for public GitHub sharing | P1 | DONE | TASK-003 |

## Completed

### ~~TASK-004~~: Prepare project for public GitHub sharing
- Added hero image to README
- Rewrote Quick Start: prerequisites, clone step, 4 client configs (Claude Code, Claude Desktop, Cursor, pip), verification step
- Removed misleading "run the server manually" step (MCP clients launch it via stdio)
- Added troubleshooting section (plugin loading, connection refused, timeouts, log location)
- Added updating instructions
- Added badges (MIT, UE 5.4+, Python 3.10+, 123 tools)
- Fixed tool counts in pyproject.toml (101/124 → 123)
- Fixed GitHub URLs: flopperam → endlessblink in pyproject.toml and both .uplugin files
- Filled CreatedByURL and SupportURL in both .uplugin files
- Implemented UNREAL_HOST/UNREAL_PORT env var support in server
- Fixed stale module docstring in it_is_unreal.py
- Recategorized get_available_materials from Skeletal Mesh to Materials
- Explained CLAUDE.md purpose for non-AI-tool users
- Aligned placeholder paths in docs/mcp-client-config.json with README
- Removed dead Blood & Dust profile link
- Three rounds of critic review: 6/10 → 8.5/10 → 10/10

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
