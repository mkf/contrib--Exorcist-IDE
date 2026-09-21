## 1. In-process MCP server framework

- [ ] 1.1 Add an `InProcessMcpServer` type under `src/mcp/` that owns a named set of `ITool` handlers and answers `tools/list` and `tools/call`; verify it compiles and a unit test maps each `ITool::spec()` to one MCP tool definition
- [ ] 1.2 Implement the handler adapter that maps `ITool::spec()` to an MCP tool definition (name, description, input schema, output schema) and `ITool::invoke()` to a structured MCP result or actionable error; verify a unit test covers success, tool error, and unknown tool name
- [ ] 1.3 Add an in-process transport to `McpClient` so a registered server is discovered without spawning a child process; verify a test connects a server and receives its `tools/list` catalogue with no `QProcess` created
- [ ] 1.4 Add a server registry that starts in-process servers with the agent platform and stops them on shutdown; verify a test shows servers stop and hosted tools are no longer callable after shutdown
- [ ] 1.5 Make server construction a thin wrapper over existing `ITool` instances and collaborators; verify a test compares thread, timer, process, and collaborator counts before and after server registration and shows no increase
- [ ] 1.6 Ensure `tools/list` reads `ITool::spec()` without side effects; verify a test shows discovery does not initialize a tool backend or perform I/O

## 2. Host aggregation

- [ ] 2.1 Implement `<server>__<tool>` namespacing with a reverse mapping to `(server, server-local tool name)`; verify a unit test covers the namespaced name, collision disambiguation, and the server-local name sent on the wire
- [ ] 2.2 Implement server-local name normalization that strips transport and vendor prefixes (`mcp_`, `mcp_mcp_`, `mcp_copilot_`, `mcp_pylance_mcp_s_`) and rejects duplicates within a server; verify a test normalizes `mcp_copilot_conta_list_containers` to `list_containers` and `mcp_mcp_docker_browser_navigate` to `navigate`, and rejects a duplicate
- [ ] 2.3 Extend `McpToolAdapter` to copy the underlying `AgentToolPermission` and record the originating server and server-local tool name; verify a test shows a dangerous hosted tool stays dangerous and the audit identity is retained
- [ ] 2.4 Implement the progressive-discovery bridge (`search_tools`, `get_tool_details`, `invoke_tool`) gated by a configurable catalogue-size threshold; verify tests cover the large-catalogue path, the small-catalogue direct path, and rejection of an out-of-catalogue invocation
- [ ] 2.5 Update bare-name matching in `src/agent/toolpresentation.cpp`, `src/agent/chat/chattoolinvocationwidget.cpp`, and `src/agent/agentcontroller.cpp` to handle namespaced hosted names; verify the existing presentation and invocation tests still pass
- [ ] 2.6 Build the discovery index lazily on first use and only when the threshold is exceeded; verify a test shows no index is built below the threshold and the index is built once above it

## 3. Core tool set

- [ ] 3.1 Ensure exactly the twelve core tools (`create_file`, `create_directory`, `read_file`, `replace_string_in_file`, `multi_replace_string_in_file`, `file_search`, `list_dir`, `get_changed_files`, `grep_search`, `semantic_search`, `search_subagent`, `get_search_view_results`) remain directly registered; verify a test asserts they are present and that no in-process MCP server advertises them

## 4. Migrate hosted servers

- [ ] 4.1 `terminal` (`runcommandtool.cpp`, `terminalsessiontools.cpp`): move `run_in_terminal`, `get_terminal_output`, `await_terminal`, `kill_terminal`, `terminal_selection`, `terminal_last_command`; verify discovery and behavior parity
- [ ] 4.2 `file-management` (`filemanagementtools.cpp`, non-core `filesystemtools.cpp`, non-core `advancedtools.cpp`): move `write_file`, `undo_file_edit`, `delete_file`, `rename_file`, `copy_file`, `watch_files`, `insert_edit_into_file`, `read_project_structure`; verify discovery and behavior parity
- [ ] 4.3 `code-intelligence` (`lsptools.cpp`, `refactortool.cpp`, `formatcodetool.cpp`, `staticanalysistool.cpp`, `treesitterquerytool.cpp`, `codegraphtool.cpp`, `changeimpacttool.cpp`, analysis tools in `devtools.cpp`, `get_errors` in `moretools.cpp`, `get_diagnostics` in `idetools.cpp`): move `rename_symbol`, `list_code_usages`, `refactor`, `format_code`, `static_analysis`, `tree_sitter_query`, `tree_sitter_parse`, `code_graph`, `analyze_change_impact`, `symbol_documentation`, `code_completion`, `performance_profile`, `generate_diagram`, `get_errors`, `get_diagnostics`; verify discovery and behavior parity
- [ ] 4.4 `orchestration` (`subagenttool.cpp`, `manage_todo_list` in `advancedtools.cpp`): move `subagent` and `manage_todo_list`; verify discovery and behavior parity
- [ ] 4.5 `memory` (`memorytool.cpp`, `managememorytool.cpp`, `managerulestool.cpp`, `agentmemorydb.cpp`, `projectbraindb.cpp`, `scratchpadtool.cpp`): move `memory`, `manage_memory`, `manage_rules`, `agent_memory_db`, `project_brain_db`, `scratchpad`; verify discovery and behavior parity
- [ ] 4.6 `editor` (`navigationtools.cpp`, `editorcontexttool.cpp`, `difftool.cpp`, `clipboardtool.cpp`): move `open_file`, `switch_header_source`, `get_editor_context`, `diff`, `clipboard`; verify discovery and behavior parity
- [ ] 4.7 `ide-commands` (`idecommandtool.cpp`, `askusertool.cpp`, `introspecttool.cpp`, `screenshottool.cpp`): move `run_ide_command`, `ask_user`, `introspect`, `screenshot`; verify discovery and behavior parity
- [ ] 4.8 `dashboard` (`dashboardtools.cpp`): move the six dashboard mission tools; verify discovery and behavior parity
- [ ] 4.9 `secrets` (`securitytools.cpp`): move `store_secret`, `get_secret`, `list_secrets`, `delete_secret`; verify discovery and behavior parity
- [ ] 4.10 `workspace` (`projectinfotool.cpp`, `workspace_config` in `sandboxtools.cpp`): move `get_project_setup_info` and `workspace_config`; verify discovery and behavior parity
- [ ] 4.11 `git` (git tools in `idetools.cpp`, `gitopstool.cpp`): move `git_status`, `git_diff`, `git_ops`; verify discovery and behavior parity
- [ ] 4.12 `github` (`githubmcptools.cpp`): move `github_issues`, `github_pr`, `github_code_search`, `github_repo`; verify discovery and behavior parity
- [ ] 4.13 `web` (`httptool.cpp`, `websearchtool.cpp`, `fetch_webpage` in `moretools.cpp`): move `http_request`, `web_search`, `fetch_webpage`; verify discovery and behavior parity
- [ ] 4.14 `browser` (no local implementation): add the server with normalized browser-automation tool definitions (`navigate`, `navigate_back`, `snapshot`, `take_screenshot`, `click`, `hover`, `type`, `fill_form`, `select_option`, `press_key`, `drag`, `file_upload`, `handle_dialog`, `evaluate`, `run_code`, `console_messages`, `network_requests`, `tabs`, `resize`, `wait_for`, `close`, `install`) and handlers that report "not available"; verify the server stays connected and each tool returns an actionable error
- [ ] 4.15 `notebook` (`notebooktools.cpp`): move `notebook_context`, `read_cell_output`, `create_notebook`, `edit_notebook_cells`, `get_notebook_summary`; verify discovery and behavior parity
- [ ] 4.16 `python` (`pythontools.cpp`): move `python_env`, `install_python_packages`, `run_python`; verify discovery and behavior parity
- [ ] 4.17 `node` (`packagemanagertools.cpp`): move `package_json_info`, `npm_run`, `install_node_packages`; verify discovery and behavior parity
- [ ] 4.18 `pylance` (no local implementation): add the server with normalized Pylance tool definitions and handlers that report "not available"; verify the server stays connected and each tool returns an actionable error
- [ ] 4.19 `sonarqube` (no local implementation): add the server with normalized SonarQube tool definitions (`analyze_file`, `list_potential_security_issues`, `exclude_from_analysis`, `setup_connected_mode`) and handlers that report "not available"; verify the server stays connected and each tool returns an actionable error
- [ ] 4.20 `docker` (`dockertools.cpp`): move the container/image/network/volume tool; verify discovery and behavior parity
- [ ] 4.21 `build-test` (`buildtools.cpp`, build tools in `devtools.cpp`, `applypatchtool.cpp`, `transactiontool.cpp`): move `build_project`, `run_tests`, `get_build_targets`, `test_failure`, `compile_and_run`, `archive`, `create_patch_file`, `apply_patch`, `begin_transaction`, `commit_transaction`, `rollback_transaction`; verify discovery and behavior parity
- [ ] 4.22 `debug` (`debugtools.cpp`): move `debug_set_breakpoint`, `debug_get_stack_trace`, `debug_get_variables`, `debug_step`; verify discovery and behavior parity
- [ ] 4.23 `database` (`databasetool.cpp`, `projectdbtool.cpp`): move `database_query` and `project_database`; verify discovery and behavior parity
- [ ] 4.24 `system` (`systemtools.cpp`, utility tools in `sandboxtools.cpp`, `image_info` in `devtools.cpp`): move `process_manager`, `network`, `json_parse_format`, `current_time`, `regex_test`, `environment_variables`, `file_content_hash`, `image_info`; verify discovery and behavior parity
- [ ] 4.25 `lua` (`luatool.cpp`, `luaagenttoolstore.cpp`): move `run_lua`, `save_lua_tool`, `list_lua_tools`, `run_lua_tool`; verify discovery and behavior parity

## 5. Integration verification

- [ ] 5.1 Build the project and run the full test suite; verify the build succeeds and all tests pass
- [ ] 5.2 Add a regression test that invokes one hosted tool per server and compares its result to the pre-migration direct implementation; verify it passes
- [ ] 5.3 Verify no non-core tool remains directly registered in `agentplatformbootstrap.cpp` and that every hosted tool is reachable through the MCP client path