## Purpose

Defines how the agent tool surface is split between a small directly registered core and a larger set of tools hosted by in-process MCP servers, and how the host discovers, namespaces, and adapts those hosted tools without changing their behavior or permissions.

## ADDED Requirements

### Requirement: Core tool set stays directly registered

The system SHALL keep exactly the following tools directly registered as in-process agent tools, and SHALL NOT expose them through an MCP server:

- Filesystem: `create_file`, `create_directory`, `read_file`, `replace_string_in_file`, `multi_replace_string_in_file`, `file_search`, `list_dir`, `get_changed_files`.
- Search: `grep_search`, `semantic_search`, `search_subagent`, `get_search_view_results`.

#### Scenario: Core tools are available without MCP

- **WHEN** the agent platform starts with no MCP server connected
- **THEN** all twelve core tools SHALL be present in the agent tool registry
- **AND** none of them SHALL be advertised by an MCP server

#### Scenario: Core tool names are not namespaced

- **WHEN** the model requests a core tool
- **THEN** the tool SHALL be invoked by its bare name (for example `read_file`)
- **AND** the name SHALL NOT carry an MCP server prefix

### Requirement: Non-core tools are hosted by in-process MCP servers

Every agent tool that is not in the core set SHALL be exposed by an in-process MCP server and SHALL NOT be directly registered in the agent tool registry.

#### Scenario: Non-core tool is not directly registered

- **WHEN** the agent platform starts
- **THEN** a non-core tool such as `run_in_terminal` SHALL NOT be present in the registry as a directly registered tool
- **AND** it SHALL become available only after its hosting MCP server is discovered

#### Scenario: Hosted tool is reachable through MCP

- **WHEN** the hosting MCP server for a non-core tool is connected
- **THEN** the tool SHALL be discoverable through the server's `tools/list` response
- **AND** invoking it SHALL route through the server's `tools/call` handling

### Requirement: In-process MCP server contract

Each in-process MCP server SHALL implement the MCP tool contract: it SHALL answer `tools/list` with typed tool definitions and SHALL answer `tools/call` by exact tool name with arguments. Each tool definition SHALL include a name, a description, and an input schema, and SHALL include an output schema when the tool produces structured output.

#### Scenario: tools/list returns typed definitions

- **WHEN** the host sends `tools/list` to an in-process MCP server
- **THEN** the server SHALL return one definition per hosted tool
- **AND** each definition SHALL include a name, a description, and an input schema

#### Scenario: tools/call dispatches by exact name

- **WHEN** the host sends `tools/call` with a tool name and arguments
- **THEN** the server SHALL invoke the handler registered for that exact name
- **AND** it SHALL return a structured result or an actionable error

#### Scenario: Unknown tool name is rejected

- **WHEN** the host sends `tools/call` with a name the server does not host
- **THEN** the server SHALL return an error identifying the unknown tool
- **AND** it SHALL NOT invoke any handler

### Requirement: Servers are grouped by responsibility

Non-core tools SHALL be grouped into MCP servers by shared responsibility rather than by inventory section, so that each server has a single, coherent purpose. A tool whose responsibility does not match its inventory section SHALL be placed with the server that matches its actual responsibility.

#### Scenario: Cohesive category maps to one server

- **WHEN** a set of tools shares one responsibility, such as terminal session management
- **THEN** those tools SHALL be hosted by a single MCP server for that responsibility

#### Scenario: Misplaced tool is regrouped

- **WHEN** a tool's inventory section does not match its responsibility, such as a browser-automation tool listed under a container-management section
- **THEN** the tool SHALL be hosted by the server matching its actual responsibility

### Requirement: Host discovers and namespaces hosted tools

The host SHALL discover each connected server's tool catalogue and SHALL expose hosted tools to the model under a namespaced name of the form `<server>__<tool>`. The host SHALL preserve the mapping from the namespaced name back to the originating server and the server-local tool name.

#### Scenario: Namespaced name is exposed

- **WHEN** a server named `terminal` hosts a tool named `run_in_terminal`
- **THEN** the model-facing tool name SHALL be `terminal__run_in_terminal`

#### Scenario: Collisions are disambiguated

- **WHEN** two servers host a tool with the same server-local name
- **THEN** the two model-facing names SHALL differ by their server prefix
- **AND** each call SHALL route to the correct server

#### Scenario: Wire call uses the server-local name

- **WHEN** the model invokes a namespaced tool
- **THEN** the host SHALL send `tools/call` to the originating server using the server-local tool name and the supplied arguments

### Requirement: Hosted tool names are normalized

Hosted tools SHALL be exposed under clean server-local names that describe the operation, and SHALL NOT retain transport or vendor prefixes such as `mcp_`, `mcp_mcp_`, `mcp_copilot_`, or `mcp_pylance_mcp_s_`. The model-facing name SHALL be the server namespace joined to the normalized server-local name.

#### Scenario: Prefixed name is normalized

- **WHEN** a tool is imported from a source whose name carries a transport prefix, such as `mcp_copilot_conta_list_containers`
- **THEN** its server-local name SHALL be `list_containers`
- **AND** its model-facing name SHALL be `docker__list_containers`

#### Scenario: Repeated prefix is collapsed

- **WHEN** a tool name carries a repeated prefix, such as `mcp_mcp_docker_browser_navigate`
- **THEN** its server-local name SHALL be `navigate`
- **AND** its model-facing name SHALL be `browser__navigate`

#### Scenario: Normalized names are unique within a server

- **WHEN** two imported names normalize to the same server-local name within one server
- **THEN** the server SHALL reject the duplicate rather than silently overwrite a handler

### Requirement: Progressive discovery for large catalogues

When the aggregate hosted catalogue is large, the host SHALL expose a compact discovery bridge instead of injecting every hosted tool schema into the model context. The bridge SHALL let the model search the catalogue, retrieve a selected tool's full definition, and invoke a selected tool.

#### Scenario: Large catalogue uses the bridge

- **WHEN** the aggregate hosted catalogue exceeds the configured size threshold
- **THEN** the host SHALL expose the discovery bridge
- **AND** it SHALL NOT inject every hosted tool schema into the model context

#### Scenario: Small catalogue is exposed directly

- **WHEN** the aggregate hosted catalogue is at or below the configured size threshold
- **THEN** the host SHALL expose the hosted tool definitions directly

#### Scenario: Bridge rejects out-of-catalogue invocation

- **WHEN** the model asks the bridge to invoke a tool that is not in the current authorized catalogue
- **THEN** the bridge SHALL reject the invocation
- **AND** it SHALL NOT call any server

### Requirement: Permissions and approvals are preserved

Each hosted tool SHALL retain the permission level of its underlying implementation, and the host SHALL apply the same approval policy to a hosted tool as it would to a directly registered tool. Approvals and audit records SHALL retain the originating server and server-local tool identity.

#### Scenario: Permission level is preserved

- **WHEN** a hosted tool's underlying implementation is classified as dangerous
- **THEN** the adapted tool SHALL be classified as dangerous
- **AND** it SHALL require the same approval as a directly registered dangerous tool

#### Scenario: Audit retains server identity

- **WHEN** a hosted tool is invoked
- **THEN** the approval and audit record SHALL identify the originating server and the server-local tool name

### Requirement: Backend behavior is preserved

Hosting a tool through MCP SHALL NOT change the tool's observable behavior. Each server-side handler SHALL delegate to the existing tool implementation, and a tool that has no existing implementation SHALL be exposed with a typed definition and a handler that reports it as unavailable rather than failing the server.

#### Scenario: Existing behavior is unchanged

- **WHEN** a hosted tool with an existing implementation is invoked
- **THEN** it SHALL produce the same result as the directly registered implementation did

#### Scenario: Unimplemented tool degrades gracefully

- **WHEN** a hosted tool has no existing implementation
- **THEN** invoking it SHALL return an actionable "not available" error
- **AND** the hosting server SHALL remain connected and continue to serve its other tools

### Requirement: In-process server lifecycle

In-process MCP servers SHALL run within the host process and SHALL NOT require a separate child process. They SHALL start with the agent platform and SHALL stop when the agent platform shuts down.

#### Scenario: No child process is spawned

- **WHEN** an in-process MCP server is started
- **THEN** it SHALL run in the host process
- **AND** it SHALL NOT spawn a separate server process

#### Scenario: Shutdown stops servers

- **WHEN** the agent platform shuts down
- **THEN** all in-process MCP servers SHALL stop
- **AND** no hosted tool SHALL remain callable

### Requirement: Hosting adds no eager initialization or background work

Creating and registering in-process MCP servers SHALL NOT initialize any collaborator, resource, connection, process, thread, or timer that the agent platform did not already initialize. Server construction SHALL be a thin wrapper over existing tool instances, and tool discovery SHALL NOT trigger side effects.

#### Scenario: No new collaborators are created

- **WHEN** the agent platform starts and registers its in-process MCP servers
- **THEN** no new manager, service, process, thread, or timer SHALL be created for hosting
- **AND** each server SHALL reference the collaborators the platform already created

#### Scenario: Discovery has no side effects

- **WHEN** the host requests `tools/list` from a server
- **THEN** the server SHALL return the catalogue without initializing a tool's backend or performing I/O

#### Scenario: No background polling

- **WHEN** in-process servers are registered
- **THEN** they SHALL NOT start background polling, timers, or watchers

#### Scenario: Discovery index is built lazily

- **WHEN** the aggregate catalogue is at or below the size threshold
- **THEN** the host SHALL NOT build a search index
- **AND** it SHALL build the index only when the threshold is exceeded and discovery is first requested