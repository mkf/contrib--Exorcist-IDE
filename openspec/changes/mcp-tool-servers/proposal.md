## Why

The agent tool registry currently hardwires every tool as an in-process `ITool` object, mixing a small, stable core (filesystem + search) with a large, heterogeneous tail (terminal, code analysis, subagents, memory, IDE integration, workspace, web/browser, notebook, Python, SonarQube, Docker, Pylance). This makes the registry a god-object, couples every tool to the host binary, and prevents tools from being discovered, versioned, or replaced independently. MCP is already the project's extension mechanism for tools; the non-core tools should use it too.

## What Changes

- Keep exactly the 12 core tools as directly registered in-process tools:
  - Filesystem (8): `create_file`, `create_directory`, `read_file`, `replace_string_in_file`, `multi_replace_string_in_file`, `file_search`, `list_dir`, `get_changed_files`.
  - Search (4): `grep_search`, `semantic_search`, `search_subagent`, `get_search_view_results`.
- **BREAKING** (internal): every other tool is removed from direct registration and instead exposed by an in-process MCP server.
- Introduce an in-process MCP server host: a C++ MCP server implementation that runs in the same process, exposes typed tools via `tools/list` and `tools/call`, and reuses the existing `ITool` implementations as server-side handlers.
- Group non-core tools into MCP servers by implementation module and shared collaborator, not by inventory section: each server is a cohesive family of tool classes that share a manager, service, or callback set. Proposed servers: `terminal`, `file-management`, `code-intelligence`, `orchestration`, `memory`, `editor`, `ide-commands`, `dashboard`, `secrets`, `workspace`, `git`, `github`, `web`, `browser`, `notebook`, `python`, `node`, `pylance`, `sonarqube`, `docker`, `build-test`, `debug`, `database`, `system`, `lua`. Regroup where a tool's inventory section does not match its implementation (for example, Docker Browser is browser automation rather than container management, and `test_failure` lives with the build/test module).
- Host-side aggregation: the MCP client discovers each server's catalogue, normalizes server-local tool names by stripping transport and vendor prefixes (`mcp_`, `mcp_mcp_`, `mcp_copilot_`, `mcp_pylance_mcp_s_`), namespaces model-facing tool names as `<server>__<tool>` to avoid collisions, and adapts them into the agent tool registry through the existing `McpToolAdapter` path.
- Progressive discovery: when the aggregate catalogue is large, the host exposes a compact search/get/invoke bridge instead of injecting every tool schema into the model context.
- Preserve permissions and backend behavior: each adapted tool keeps its `AgentToolPermission`, and each server-side handler delegates to the existing implementation unchanged.
- No new initialization or background work: servers are thin wrappers over the tool instances and collaborators the platform already creates. Hosting adds no new processes, threads, timers, or connections, `tools/list` has no side effects, and the discovery index is built lazily only above the threshold.
- Categories with no existing implementation (SonarQube, Pylance) get server scaffolding with typed tool definitions and stub handlers — no worse than today, where they are absent.

## Capabilities

### New Capabilities
- `mcp-tool-hosting`: Defines the core-vs-hosted tool split, the in-process MCP server contract, host-side discovery/namespacing/adaptation, progressive discovery, and permission preservation.

### Modified Capabilities
<!-- None: no existing capability's requirements change. -->

## Impact

- New: in-process MCP server framework under `src/mcp/` (server host, tool handler adapter, server registry).
- `src/agent/agentplatformbootstrap.cpp`: stop registering non-core tools directly; register MCP servers instead.
- `src/agent/itool.h` / `ToolRegistry`: contract unchanged, but the registry is populated from MCP discovery for non-core tools.
- `src/mcp/mcpclient.*`, `src/mcp/mcptooladapter.h`: extended for in-process transport and namespacing.
- `src/mainwindow_docks.cpp`: the MCP tool registration path becomes the primary path for non-core tools.
- Tests: new coverage for server grouping, `tools/list` output, namespacing, and permission preservation.