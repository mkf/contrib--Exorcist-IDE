## Context

See [`proposal.md`](proposal.md) for motivation. The relevant current state:

- [`src/agent/itool.h`](../../../src/agent/itool.h) defines `ITool` (`spec()` + `invoke()`) and `ToolRegistry`, which owns every tool as a `std::unique_ptr<ITool>`.
- [`src/agent/agentplatformbootstrap.cpp`](../../../src/agent/agentplatformbootstrap.cpp) constructs and registers ~80 tools directly, wiring IDE callbacks into each constructor.
- [`src/mcp/mcpclient.h`](../../../src/mcp/mcpclient.h) and [`src/mcp/mcptooladapter.h`](../../../src/mcp/mcptooladapter.h) already implement an MCP client and an `ITool` adapter for remote (stdio) servers. [`src/mainwindow_docks.cpp`](../../../src/mainwindow_docks.cpp) already registers discovered MCP tools into the registry as `mcp_<server>_<tool>`.
- [`server/mcpbridgeservice.cpp`](../../../server/mcpbridgeservice.cpp) manages out-of-process MCP servers in the ExoBridge daemon.

The existing MCP path is out-of-process and used for third-party servers. This change adds an **in-process** MCP server path for the project's own non-core tools, reusing the same client/adapter contract.

## Goals / Non-Goals

**Goals:**

- Split the tool surface into a fixed 12-tool core (direct) and a hosted tail (MCP).
- Provide an in-process MCP server host that reuses existing `ITool` implementations as handlers.
- Group hosted tools by implementation module and shared collaborator, not by inventory section.
- Reuse the existing MCP client + `McpToolAdapter` path for discovery and invocation.
- Preserve permissions, approvals, and observable behavior.

**Non-Goals:**

- Implementing missing backends (SonarQube, Pylance, browser automation). These get typed definitions and graceful "not available" handlers.
- Changing the MCP wire protocol or the out-of-process ExoBridge path.
- Changing the model-provider interface or adding provider-native deferred tool loading.
- Rewriting the existing `ITool` implementations.

## Decisions

### Decision: In-process transport, not child processes

In-process servers implement the MCP `tools/list` / `tools/call` contract directly in C++ and are registered with the host's MCP client through an in-process transport, rather than being spawned as child processes.

- **Why:** the hosted tools depend on live IDE services (terminal sessions, LSP, editor state) that cannot cross a process boundary without a large IPC surface. The user explicitly chose in-process.
- **Alternative considered:** one stdio binary per server. Rejected: it would require serializing IDE state and would not be "no further from a working state" than today.

### Decision: A server is a named collection of `ITool` handlers

An in-process MCP server is a small object that owns a set of `ITool` instances and answers `tools/list` (mapping each `ITool::spec()` to an MCP tool definition) and `tools/call` (dispatching to `ITool::invoke()`). This keeps the existing implementations untouched and makes the server a thin protocol shell.

- **Why:** minimal change to existing code; the `ITool` contract already carries name, description, input schema, output schema, and permission.
- **Alternative considered:** a new handler interface. Rejected: it would duplicate `ITool` and force rewrites.

### Decision: Thin, lazy servers with no new resources

Servers hold references to the `ITool` instances and collaborators the platform already created; construction allocates only the server object and its name→handler map. The in-process transport is a direct call, not a socket, thread, or process. `tools/list` reads `ITool::spec()` and performs no I/O or backend initialization. The progressive-discovery index is built lazily on first use and only when the threshold is exceeded.

- **Why:** the abstraction must not change the platform's initialization order or resource profile. Hosting is a naming and dispatch layer, not a new subsystem.
- **Alternative considered:** eagerly building a full catalogue index at startup. Rejected: it duplicates schemas in memory and adds startup cost for no benefit below the threshold.
- **Alternative considered:** a loopback socket or worker thread for the in-process transport. Rejected: it adds threads, buffers, and latency for no isolation benefit.

### Decision: Group by implementation module and shared collaborator

The grouping follows the existing implementation, not the inventory sections. Each server corresponds to a cohesive family of tool classes that share a collaborator (a manager, a service, or a callback set) and live in the same source module. This keeps a server's handlers constructible from one dependency set and prevents a server from mixing unrelated collaborators.

| Server | Implementation module(s) | Shared collaborator | Tools |
|--------|--------------------------|---------------------|-------|
| `terminal` | `runcommandtool.cpp`, `terminalsessiontools.cpp` | `TerminalSessionManager`, `QProcess` | `run_in_terminal`, `get_terminal_output`, `await_terminal`, `kill_terminal`, `terminal_selection`, `terminal_last_command` |
| `file-management` | `filemanagementtools.cpp`, `filesystemtools.cpp` (non-core), `advancedtools.cpp` (non-core) | filesystem access | `write_file`, `undo_file_edit`, `delete_file`, `rename_file`, `copy_file`, `watch_files`, `insert_edit_into_file`, `read_project_structure` |
| `code-intelligence` | `lsptools.cpp`, `refactortool.cpp`, `formatcodetool.cpp`, `staticanalysistool.cpp`, `treesitterquerytool.cpp`, `codegraphtool.cpp`, `changeimpacttool.cpp`, `devtools.cpp` (analysis), `moretools.cpp` (`get_errors`), `idetools.cpp` (`get_diagnostics`) | LSP / tree-sitter / analysis callbacks | `rename_symbol`, `list_code_usages`, `refactor`, `format_code`, `static_analysis`, `tree_sitter_query`, `tree_sitter_parse`, `code_graph`, `analyze_change_impact`, `symbol_documentation`, `code_completion`, `performance_profile`, `generate_diagram`, `get_errors`, `get_diagnostics` |
| `orchestration` | `subagenttool.cpp`, `advancedtools.cpp` (`manage_todo_list`) | orchestrator + tool registry | `subagent`, `manage_todo_list` |
| `memory` | `memorytool.cpp`, `managememorytool.cpp`, `managerulestool.cpp`, `agentmemorydb.cpp`, `projectbraindb.cpp`, `scratchpadtool.cpp` | `ProjectBrainService` / persistent storage | `memory`, `manage_memory`, `manage_rules`, `agent_memory_db`, `project_brain_db`, `scratchpad` |
| `editor` | `navigationtools.cpp`, `editorcontexttool.cpp`, `difftool.cpp`, `clipboardtool.cpp` | editor state callbacks | `open_file`, `switch_header_source`, `get_editor_context`, `diff`, `clipboard` |
| `ide-commands` | `idecommandtool.cpp`, `askusertool.cpp`, `introspecttool.cpp`, `screenshottool.cpp` | host command/UI callbacks | `run_ide_command`, `ask_user`, `introspect`, `screenshot` |
| `dashboard` | `dashboardtools.cpp` | `AgentUIBus` | `create_dashboard_mission`, `update_dashboard_step`, `update_dashboard_metric`, `add_dashboard_log`, `add_dashboard_artifact`, `complete_dashboard_mission` |
| `secrets` | `securitytools.cpp` | `SecureKeyStorage` | `store_secret`, `get_secret`, `list_secrets`, `delete_secret` |
| `workspace` | `projectinfotool.cpp`, `sandboxtools.cpp` (`workspace_config`) | workspace root | `get_project_setup_info`, `workspace_config` |
| `git` | `idetools.cpp` (git), `gitopstool.cpp` | git callbacks | `git_status`, `git_diff`, `git_ops` |
| `github` | `githubmcptools.cpp` | GitHub HTTP | `github_issues`, `github_pr`, `github_code_search`, `github_repo` |
| `web` | `httptool.cpp`, `websearchtool.cpp`, `moretools.cpp` (`fetch_webpage`) | HTTP | `http_request`, `web_search`, `fetch_webpage` |
| `browser` | (no local implementation) | browser automation | `navigate`, `navigate_back`, `snapshot`, `take_screenshot`, `click`, `hover`, `type`, `fill_form`, `select_option`, `press_key`, `drag`, `file_upload`, `handle_dialog`, `evaluate`, `run_code`, `console_messages`, `network_requests`, `tabs`, `resize`, `wait_for`, `close`, `install` |
| `notebook` | `notebooktools.cpp` | `NotebookManager` | `notebook_context`, `read_cell_output`, `create_notebook`, `edit_notebook_cells`, `get_notebook_summary` |
| `python` | `pythontools.cpp` | `TerminalSessionManager` | `python_env`, `install_python_packages`, `run_python` |
| `node` | `packagemanagertools.cpp` | `TerminalSessionManager` | `package_json_info`, `npm_run`, `install_node_packages` |
| `pylance` | (no local implementation) | Python language server | Pylance document/settings/environment/import/syntax/refactor/run tools |
| `sonarqube` | (no local implementation) | SonarQube service | `analyze_file`, `list_potential_security_issues`, `exclude_from_analysis`, `setup_connected_mode` |
| `docker` | `dockertools.cpp` | `QProcess` | `docker` (container/image/network/volume operations) |
| `build-test` | `buildtools.cpp`, `devtools.cpp` (build), `applypatchtool.cpp`, `transactiontool.cpp` | build/test callbacks | `build_project`, `run_tests`, `get_build_targets`, `test_failure`, `compile_and_run`, `archive`, `create_patch_file`, `apply_patch`, `begin_transaction`, `commit_transaction`, `rollback_transaction` |
| `debug` | `debugtools.cpp` | debug adapter callbacks | `debug_set_breakpoint`, `debug_get_stack_trace`, `debug_get_variables`, `debug_step` |
| `database` | `databasetool.cpp`, `projectdbtool.cpp` | SQLite / DB drivers | `database_query`, `project_database` |
| `system` | `systemtools.cpp`, `sandboxtools.cpp` (utilities), `devtools.cpp` (`image_info`) | host/process | `process_manager`, `network`, `json_parse_format`, `current_time`, `regex_test`, `environment_variables`, `file_content_hash`, `image_info` |
| `lua` | `luatool.cpp`, `luaagenttoolstore.cpp` | Lua sandbox | `run_lua`, `save_lua_tool`, `list_lua_tools`, `run_lua_tool` |

Notable regroupings and their rationale:

- **Docker Browser → `browser`.** It is Playwright browser automation that happens to run in a container; its responsibility is browser control, not container management.
- **`generate_diagram` → `code-intelligence`.** Diagram rendering is not Python environment management.
- **`github_repo` → `github`.** Remote GitHub access is a distinct service responsibility, not workspace structure.
- **`project_brain_db` → `memory`.** It backs the project brain's persistent knowledge, so it belongs with memory management rather than generic databases.
- **`test_failure` → `build-test`.** It reports the outcome of a test run, so it belongs with the build/test module rather than code analysis.
- **`node` split from `python`.** `packagemanagertools.cpp` and `pythontools.cpp` are separate modules with separate package ecosystems, so they are separate servers even though both use `TerminalSessionManager`.

The exact server list is derived from the implementation modules and may be refined during implementation; the invariant is one cohesive collaborator family per server.

### Decision: Normalize server-local tool names

Server-local tool names are normalized to a clean operation name: transport and vendor prefixes (`mcp_`, `mcp_mcp_`, `mcp_copilot_`, `mcp_pylance_mcp_s_`) are stripped, and the name is reduced to the operation (for example `mcp_copilot_conta_list_containers` → `list_containers`, `mcp_mcp_docker_browser_navigate` → `navigate`). The model-facing name is then `<server>__<normalized name>`.

- **Why:** the imported names encode the transport and the source server, which is redundant once the tool is hosted by a named in-process server. Clean names are stable, readable, and collision-free within a server.
- **Alternative considered:** keep the imported names verbatim. Rejected: `mcp_mcp_...` names are noise and leak the transport into the model interface.

### Decision: Namespacing at the host boundary

Model-facing names are `<server>__<tool>` (double underscore), e.g. `terminal__run_in_terminal`. The host keeps a mapping to `(server, server-local tool name)` and sends the server-local name on the wire.

- **Why:** MCP tool names are unique only within a server; the host aggregates many servers. Double underscore avoids ambiguity with single-underscore tool names.
- **Alternative considered:** `mcp_<server>_<tool>` (the existing remote-server convention). Rejected for the in-process path because it collides with the existing remote prefix and is harder to parse unambiguously.

### Decision: Progressive discovery above a threshold

When the aggregate hosted catalogue exceeds a configured size threshold, the host exposes a compact bridge (`search_tools`, `get_tool_details`, `invoke_tool`) instead of injecting every schema. Below the threshold, hosted definitions are exposed directly.

- **Why:** the hosted tail is large; injecting every schema consumes model context. The research brief identifies progressive discovery as the current best practice.
- **Alternative considered:** always inject everything. Rejected: context cost.
- **Alternative considered:** provider-native deferred loading. Deferred: the provider interface does not expose it yet; the host-side bridge is portable.

### Decision: Permission and identity preservation

`McpToolAdapter` copies the underlying `ITool`'s `AgentToolPermission` into the adapted `ToolSpec`, and the host records `(server, server-local tool)` on approvals and audit entries.

- **Why:** hosting must not silently downgrade a dangerous tool to safe, and audit must remain attributable.

## Risks / Trade-offs

- **Behavior drift when moving a tool behind MCP.** → The server handler delegates to the existing `ITool` unchanged; add a regression test per server that compares a hosted call to the direct implementation.
- **Namespacing breaks existing references.** → `toolpresentation.cpp`, `chattoolinvocationwidget.cpp`, and `agentcontroller.cpp` match on bare tool names. → Update these to match on the server-local name (or the namespaced name) and add a test.
- **Progressive discovery hides tools from the model.** → Keep the threshold configurable and expose the full catalogue when below it; log when the bridge is active.
- **In-process servers share the host event loop.** → A slow handler blocks the UI. → Keep the existing per-tool `timeoutMs` and `cancellable` semantics; do not add new blocking work.
- **Hosting duplicates schemas or eagerly initializes backends.** → Servers reference existing `ITool` instances and collaborators; `tools/list` reads `spec()` without side effects; the discovery index is built lazily and only above the threshold. → Add a test that compares resource counts before and after server registration and a test that discovery does not initialize a backend.
- **Stub servers (SonarQube, Pylance) may look functional.** → Their handlers return an explicit "not available" error and the server stays connected.

## Migration Plan

1. Add the in-process MCP server host and register it with the MCP client.
2. Move tools server-by-server, starting with `terminal` (self-contained) and ending with the stub servers.
3. For each server: remove the direct registrations from `agentplatformbootstrap.cpp`, add the server, and verify the hosted tool is discovered and callable.
4. Update name-matching call sites for namespaced names.
5. Rollback: re-add the direct registrations for a server and drop its server object; the two paths are independent.

## Open Questions

- The exact size threshold for progressive discovery is a tuning value; it can be set from the existing feature-flag/settings mechanism without changing the specs.
- Whether `python` and `pylance` should eventually merge into one Python server is a grouping refinement that does not change the specs or the task breakdown.