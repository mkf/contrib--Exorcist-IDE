#pragma once

#include "../../mcp/inprocessmcpserver.h"

#include <QString>

#include <memory>

struct AgentToolCallbacks;
class AgentController;
class AgentOrchestrator;
class AgentUIBus;
class ContextBuilder;
class IFileSystem;
class IProcess;
class ProjectBrainService;
class TerminalSessionManager;
class ToolRegistry;
class TransactionStore;
class QObject;

// ── In-process MCP tool servers ───────────────────────────────────────────────
//
// Each server owns one cohesive family of ITool handlers and populates itself
// from the collaborators the platform already created. Construction is a thin
// wrapper: no new managers, processes, threads, or timers.

// Terminal session lifecycle.
class TerminalToolServer : public InProcessMcpServer
{
public:
    TerminalToolServer(IProcess *process,
                       const QString &workspaceRoot,
                       TerminalSessionManager *sessionManager,
                       const AgentToolCallbacks &callbacks);
};

// File operations that are not part of the core filesystem set.
class FileManagementToolServer : public InProcessMcpServer
{
public:
    FileManagementToolServer(IFileSystem *fileSystem,
                             const QString &workspaceRoot,
                             AgentController *controller,
                             std::shared_ptr<TransactionStore> txStore);
};

// Subagents and task tracking.
class OrchestrationToolServer : public InProcessMcpServer
{
public:
    OrchestrationToolServer(AgentOrchestrator *orchestrator,
                            ToolRegistry *toolRegistry,
                            ContextBuilder *contextBuilder,
                            const QString &appDataPath);
};

// Persistent knowledge: memory files, scratchpad, databases, brain rules.
class MemoryToolServer : public InProcessMcpServer
{
public:
    MemoryToolServer(ProjectBrainService *brainService,
                     const QString &appDataPath);
};

// Code intelligence: diagnostics, LSP, tree-sitter, refactoring, analysis.
class CodeIntelligenceToolServer : public InProcessMcpServer
{
public:
    explicit CodeIntelligenceToolServer(const AgentToolCallbacks &callbacks);
};

// Debug adapter control.
class DebugToolServer : public InProcessMcpServer
{
public:
    explicit DebugToolServer(const AgentToolCallbacks &callbacks);
};

// Editor state: navigation, context, clipboard, diff.
class EditorToolServer : public InProcessMcpServer
{
public:
    EditorToolServer(const AgentToolCallbacks &callbacks,
                     const QString &workspaceRoot);
};

// IDE command execution, user prompts, introspection, screenshots.
class IdeCommandToolServer : public InProcessMcpServer
{
public:
    explicit IdeCommandToolServer(const AgentToolCallbacks &callbacks);
};

// Build, test, packaging, patches, and file transactions.
class BuildTestToolServer : public InProcessMcpServer
{
public:
    BuildTestToolServer(const AgentToolCallbacks &callbacks,
                        IFileSystem *fileSystem,
                        std::shared_ptr<TransactionStore> txStore);
};

// Project/workspace structure and setup.
class WorkspaceToolServer : public InProcessMcpServer
{
public:
    WorkspaceToolServer(const QString &workspaceRoot, IFileSystem *fileSystem);
};

// Git status and diff.
class GitToolServer : public InProcessMcpServer
{
public:
    explicit GitToolServer(const AgentToolCallbacks &callbacks);
};

// Remote GitHub access.
class GithubToolServer : public InProcessMcpServer
{
public:
    GithubToolServer();
};

// Web fetch, search, and HTTP.
class WebToolServer : public InProcessMcpServer
{
public:
    WebToolServer();
};

// Browser automation (no local backend).
class BrowserToolServer : public InProcessMcpServer
{
public:
    BrowserToolServer();
};

// Jupyter notebook lifecycle.
class NotebookToolServer : public InProcessMcpServer
{
public:
    explicit NotebookToolServer(QObject *parent);
};

// Python runtime environment.
class PythonToolServer : public InProcessMcpServer
{
public:
    explicit PythonToolServer(TerminalSessionManager *sessionManager);
};

// Node/package management.
class NodeToolServer : public InProcessMcpServer
{
public:
    explicit NodeToolServer(TerminalSessionManager *sessionManager);
};

// Python language server (no local backend).
class PylanceToolServer : public InProcessMcpServer
{
public:
    PylanceToolServer();
};

// SonarQube service (no local backend).
class SonarQubeToolServer : public InProcessMcpServer
{
public:
    SonarQubeToolServer();
};

// Docker container management.
class DockerToolServer : public InProcessMcpServer
{
public:
    explicit DockerToolServer(IProcess *process);
};

// Local and project databases.
class DatabaseToolServer : public InProcessMcpServer
{
public:
    DatabaseToolServer();
};

// Host/process/environment utilities.
class SystemToolServer : public InProcessMcpServer
{
public:
    SystemToolServer();
};

// Sandboxed Lua tooling.
class LuaToolServer : public InProcessMcpServer
{
public:
    explicit LuaToolServer(const AgentToolCallbacks &callbacks);
};

// Agent dashboard (live operational UI).
class DashboardToolServer : public InProcessMcpServer
{
public:
    explicit DashboardToolServer(AgentUIBus *uiBus);
};

// Secure key storage.
class SecretsToolServer : public InProcessMcpServer
{
public:
    explicit SecretsToolServer(const AgentToolCallbacks &callbacks);
};