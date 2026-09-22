#include "agentplatformbootstrap.h"

#include "mcpservers/agenttoolservers.h"

#include "agentcontroller.h"
#include "agentorchestrator.h"
#include "agentproviderregistry.h"
#include "agentrequestrouter.h"
#include "braincontextbuilder.h"
#include "chatsessionservice.h"
#include "contextbuilder.h"
#include "iagentplugin.h"
#include "iagentsettingspageprovider.h"
#include "ichatsessionimporter.h"
#include "iproviderauthintegration.h"
#include "itool.h"
#include "memorysuggestionengine.h"
#include "projectbrainservice.h"
#include "sessionstore.h"
#include "toolapprovalservice.h"
#include "tools/advancedtools.h"
#include "tools/applypatchtool.h"
#include "tools/filesystemtools.h"
#include "tools/idetools.h"
#include "tools/memorytool.h"
#include "tools/moretools.h"
#include "tools/runcommandtool.h"
#include "tools/searchworkspacetool.h"
#include "tools/semanticsearchtool.h"
#include "tools/websearchtool.h"
#include "tools/debugtools.h"
#include "tools/screenshottool.h"
#include "tools/introspecttool.h"
#include "tools/httptool.h"
#include "tools/luatool.h"
#include "tools/luaagenttoolstore.h"
#include "tools/codegraphtool.h"
#include "tools/buildtools.h"
#include "tools/navigationtools.h"
#include "tools/formatcodetool.h"
#include "tools/refactortool.h"
#include "tools/askusertool.h"
#include "tools/editorcontexttool.h"
#include "tools/changeimpacttool.h"
#include "tools/scratchpadtool.h"
#include "tools/managerulestool.h"
#include "tools/managememorytool.h"
#include "tools/terminalsessiontools.h"
#include "tools/pythontools.h"
#include "tools/packagemanagertools.h"
#include "tools/subagenttool.h"
#include "tools/lsptools.h"
#include "tools/projectinfotool.h"
#include "tools/filemanagementtools.h"
#include "tools/clipboardtool.h"
#include "tools/difftool.h"
#include "tools/databasetool.h"
#include "tools/agentmemorydb.h"
#include "tools/projectdbtool.h"
#include "tools/projectbraindb.h"
#include "tools/systemtools.h"
#include "tools/notebooktools.h"
#include "tools/githubmcptools.h"
#include "tools/idecommandtool.h"
#include "tools/sandboxtools.h"
#include "tools/devtools.h"
#include "tools/treesitterquerytool.h"
#include "tools/dashboardtools.h"
#include "tools/dockertools.h"
#include "tools/securitytools.h"
#include "tools/transactiontool.h"
#include "ui/agentuibus.h"
#include "diagnosticsnotifier.h"
#include "terminalsessionmanager.h"
#include "workspacecontextdetector.h"
#include "../core/qtprocess.h"
#include "../mcp/inprocessmcpserver.h"
#include "../pluginmanager.h"
#include "../serviceregistry.h"

#include <QStandardPaths>

#include <memory>

AgentPlatformBootstrap::AgentPlatformBootstrap(AgentOrchestrator *orchestrator,
                                               ServiceRegistry *services,
                                               IFileSystem *fileSystem,
                                               QObject *parent)
    : QObject(parent)
    , m_orchestrator(orchestrator)
    , m_services(services)
    , m_fileSystem(fileSystem)
    , m_process(std::make_unique<QtProcess>())
{
}

AgentPlatformBootstrap::~AgentPlatformBootstrap() = default;

QList<InProcessMcpServer *> AgentPlatformBootstrap::mcpServers() const
{
    QList<InProcessMcpServer *> servers;
    servers.reserve(static_cast<int>(m_mcpServers.size()));
    for (const auto &server : m_mcpServers)
        servers.append(server.get());
    return servers;
}

ITool *AgentPlatformBootstrap::findTool(const QString &name) const
{
    if (m_toolRegistry) {
        if (ITool *t = findTool(name))
            return t;
    }
    for (const auto &server : m_mcpServers) {
        if (ITool *t = server->tool(name))
            return t;
    }
    return nullptr;
}

void AgentPlatformBootstrap::initialize(const Callbacks &callbacks)
{
    m_callbacks = callbacks;

    m_toolRegistry = new ToolRegistry(this);
    m_contextBuilder = new ContextBuilder(this);
    m_sessionStore = new SessionStore(this);
    m_sessionManager = new TerminalSessionManager({}, this);
    m_agentController = new AgentController(m_orchestrator, m_toolRegistry, m_contextBuilder, this);
    m_agentController->setSessionStore(m_sessionStore);

    // ── New platform services ─────────────────────────────────────────────
    m_providerRegistry = new AgentProviderRegistry(this);
    m_requestRouter    = new AgentRequestRouter(m_providerRegistry, this);
    m_chatSessionService = new ChatSessionService(m_sessionStore, this);
    m_toolApprovalService = new ToolApprovalService(this);

    // Wire the approval service into the controller
    m_agentController->setToolApprovalService(m_toolApprovalService);

    // Project brain (persistent workspace knowledge)
    m_brainService = new ProjectBrainService(this);
    m_brainBuilder = new BrainContextBuilder(m_brainService, this);
    m_memorySuggestionEngine = new MemorySuggestionEngine(m_brainService, this);
    m_agentController->setBrainContextBuilder(m_brainBuilder);

    // Agent UI event bus (dashboard protocol)
    m_uiBus = new AgentUIBus(this);

    m_contextBuilder->setFileSystem(m_fileSystem);
    m_contextBuilder->setOpenFilesGetter(m_callbacks.openFilesGetter);
    m_contextBuilder->setGitStatusGetter(m_callbacks.gitStatusGetter);
    m_contextBuilder->setTerminalOutputGetter(m_callbacks.terminalOutputGetter);
    m_contextBuilder->setDiagnosticsGetter(m_callbacks.diagnosticsGetter);

    // Adapt per-file gitDiffGetter to no-arg getter (full working tree diff)
    if (m_callbacks.gitDiffGetter) {
        m_contextBuilder->setGitDiffGetter([cb = m_callbacks.gitDiffGetter]() -> QString {
            return cb(QString()); // empty path = full diff
        });
    }

    if (m_services) {
        m_services->registerService(QStringLiteral("toolRegistry"), m_toolRegistry);
        m_services->registerService(QStringLiteral("contextBuilder"), m_contextBuilder);
        m_services->registerService(QStringLiteral("agentController"), m_agentController);
        m_services->registerService(QStringLiteral("sessionStore"), m_sessionStore);
        m_services->registerService(QStringLiteral("agentProviderRegistry"), m_providerRegistry);
        m_services->registerService(QStringLiteral("agentRequestRouter"), m_requestRouter);
        m_services->registerService(QStringLiteral("chatSessionService"), m_chatSessionService);
        m_services->registerService(QStringLiteral("toolApprovalService"), m_toolApprovalService);
        m_services->registerService(QStringLiteral("projectBrainService"), m_brainService);
        m_services->registerService(QStringLiteral("agentUIBus"), m_uiBus);
    }
}

void AgentPlatformBootstrap::registerCoreTools(const QString &workspaceRoot)
{
    if (!m_toolRegistry) {
        return;
    }

    // ── Core tools (directly registered, never hosted) ───────────────────
    // Filesystem: create_file, create_directory, read_file,
    //   replace_string_in_file, multi_replace_string_in_file, file_search,
    //   list_dir, get_changed_files.
    // Search: grep_search, semantic_search.
    m_toolRegistry->registerTool(std::make_unique<ReadFileTool>(m_fileSystem));
    m_toolRegistry->registerTool(std::make_unique<ListFilesTool>(m_fileSystem));

    // ── Atomic transaction support wired into write tools ─────────────────
    auto txStore = std::make_shared<TransactionStore>();
    {
        auto writeTool = std::make_unique<WriteFileTool>(m_fileSystem);
        writeTool->setTransactionStore(txStore);
        m_toolRegistry->registerTool(std::move(writeTool));
    }
    m_toolRegistry->registerTool(std::make_unique<ReplaceStringTool>());
    m_toolRegistry->registerTool(std::make_unique<MultiReplaceStringTool>());
    m_toolRegistry->registerTool(std::make_unique<SearchWorkspaceTool>(workspaceRoot));
    m_toolRegistry->registerTool(std::make_unique<SemanticSearchTool>(workspaceRoot));
    m_toolRegistry->registerTool(std::make_unique<FileSearchTool>(workspaceRoot));
    m_toolRegistry->registerTool(std::make_unique<CreateDirectoryTool>());
    m_toolRegistry->registerTool(std::make_unique<GetChangedFilesTool>(m_callbacks.changedFilesGetter));

    // ── Hosted MCP servers (non-core tools) ──────────────────────────────
    // Each server owns one cohesive tool family and is constructed from the
    // collaborators the platform already created.
    const QString appDataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    m_mcpServers.push_back(std::make_unique<TerminalToolServer>(
        m_process.get(), workspaceRoot, m_sessionManager, m_callbacks));
    m_mcpServers.push_back(std::make_unique<FileManagementToolServer>(
        m_fileSystem, workspaceRoot, m_agentController, txStore));
    m_mcpServers.push_back(std::make_unique<OrchestrationToolServer>(
        m_orchestrator, m_toolRegistry, m_contextBuilder, appDataPath));
    m_mcpServers.push_back(std::make_unique<MemoryToolServer>(
        m_brainService, appDataPath));
    m_mcpServers.push_back(std::make_unique<CodeIntelligenceToolServer>(m_callbacks));
    m_mcpServers.push_back(std::make_unique<DebugToolServer>(m_callbacks));
    m_mcpServers.push_back(std::make_unique<EditorToolServer>(m_callbacks, workspaceRoot));
    m_mcpServers.push_back(std::make_unique<IdeCommandToolServer>(m_callbacks));
    m_mcpServers.push_back(std::make_unique<BuildTestToolServer>(
        m_callbacks, m_fileSystem, txStore));
    m_mcpServers.push_back(std::make_unique<WorkspaceToolServer>(
        workspaceRoot, m_fileSystem));
    m_mcpServers.push_back(std::make_unique<GitToolServer>(m_callbacks));
    m_mcpServers.push_back(std::make_unique<GithubToolServer>());
    m_mcpServers.push_back(std::make_unique<WebToolServer>());
    m_mcpServers.push_back(std::make_unique<BrowserToolServer>());
    m_mcpServers.push_back(std::make_unique<NotebookToolServer>(this));
    m_mcpServers.push_back(std::make_unique<PythonToolServer>(m_sessionManager));
    m_mcpServers.push_back(std::make_unique<NodeToolServer>(m_sessionManager));
    m_mcpServers.push_back(std::make_unique<PylanceToolServer>());
    m_mcpServers.push_back(std::make_unique<SonarQubeToolServer>());
    m_mcpServers.push_back(std::make_unique<DockerToolServer>(m_process.get()));
    m_mcpServers.push_back(std::make_unique<DatabaseToolServer>());
    m_mcpServers.push_back(std::make_unique<SystemToolServer>());
    m_mcpServers.push_back(std::make_unique<LuaToolServer>(m_callbacks));
    if (m_uiBus)
        m_mcpServers.push_back(std::make_unique<DashboardToolServer>(m_uiBus));
    m_mcpServers.push_back(std::make_unique<SecretsToolServer>(m_callbacks));

    // ── LSP diagnostics real-time push ────────────────────────────────────
    m_diagnosticsNotifier = std::make_unique<DiagnosticsNotifier>(this);
    if (m_agentController) {
        m_agentController->setDiagnosticsNotifier(m_diagnosticsNotifier.get());
    }

    setWorkspaceRoot(workspaceRoot);
}

void AgentPlatformBootstrap::registerPluginProviders(PluginManager *pluginManager)
{
    if (!pluginManager || !m_orchestrator) {
        return;
    }

    for (QObject *obj : pluginManager->pluginObjects()) {
        // ── AI provider plugins (IAgentPlugin) ────────────────────────
        if (auto *agentPlugin = qobject_cast<IAgentPlugin *>(obj)) {
            const auto providers = agentPlugin->createProviders(m_orchestrator);
            for (IAgentProvider *provider : providers) {
                m_orchestrator->registerProvider(provider);
                if (m_providerRegistry)
                    m_providerRegistry->registerProvider(provider);
            }

            if (auto *importer = qobject_cast<IChatSessionImporter *>(obj))
                m_sessionImporters.append(importer);

            if (auto *auth = qobject_cast<IProviderAuthIntegration *>(obj))
                m_authIntegrations.append(auth);

            if (auto *settings = qobject_cast<IAgentSettingsPageProvider *>(obj))
                m_settingsPages.append(settings);
        }

        // ── Agent tool plugins (IAgentToolPlugin) — any plugin may implement ──
        if (auto *toolPlugin = qobject_cast<IAgentToolPlugin *>(obj)) {
            auto tools = toolPlugin->createTools();
            for (auto &tool : tools) {
                if (m_toolRegistry)
                    m_toolRegistry->registerTool(std::move(tool));
            }
        }
    }

    // ── Register C ABI plugin providers ───────────────────────────────
    for (IAgentProvider *provider : pluginManager->cabiProviders()) {
        m_orchestrator->registerProvider(provider);
        if (m_providerRegistry)
            m_providerRegistry->registerProvider(provider);
    }
}

void AgentPlatformBootstrap::setWorkspaceRoot(const QString &root)
{
    if (m_contextBuilder) {
        m_contextBuilder->setWorkspaceRoot(root);
    }

    if (m_brainService) {
        m_brainService->load(root);
    }

    if (!m_toolRegistry) {
        return;
    }

    auto *searchTool = dynamic_cast<SearchWorkspaceTool *>(findTool(QStringLiteral("grep_search")));
    if (searchTool) {
        searchTool->setWorkspaceRoot(root);
    }

    auto *semSearchTool = dynamic_cast<SemanticSearchTool *>(findTool(QStringLiteral("semantic_search")));
    if (semSearchTool) {
        semSearchTool->setWorkspaceRoot(root);
    }

    auto *fileSearchTool = dynamic_cast<FileSearchTool *>(findTool(QStringLiteral("file_search")));
    if (fileSearchTool) {
        fileSearchTool->setWorkspaceRoot(root);
    }

    auto *runCommandTool = dynamic_cast<RunCommandTool *>(findTool(QStringLiteral("run_in_terminal")));
    if (runCommandTool) {
        runCommandTool->setWorkingDirectory(root);
    }

    // Update terminal session manager working directory
    if (m_sessionManager) {
        m_sessionManager->setWorkingDirectory(root);
    }

    // ── Detect workspace contexts and update tool filtering ────────
    const QSet<QString> contexts = WorkspaceContextDetector::detect(root);
    m_toolRegistry->setActiveContexts(contexts);

    auto *readTool = dynamic_cast<ReadFileTool *>(findTool(QStringLiteral("read_file")));
    if (readTool) {
        readTool->setWorkspaceRoot(root);
    }

    auto *listTool = dynamic_cast<ListFilesTool *>(findTool(QStringLiteral("list_dir")));
    if (listTool) {
        listTool->setWorkspaceRoot(root);
    }

    auto *writeTool = dynamic_cast<WriteFileTool *>(findTool(QStringLiteral("create_file")));
    if (writeTool) {
        writeTool->setWorkspaceRoot(root);
    }

    auto *projectTool = dynamic_cast<ReadProjectStructureTool *>(findTool(QStringLiteral("read_project_structure")));
    if (projectTool) {
        projectTool->setWorkspaceRoot(root);
    }

    auto *setupInfoTool = dynamic_cast<GetProjectSetupInfoTool *>(findTool(QStringLiteral("get_project_setup_info")));
    if (setupInfoTool) {
        setupInfoTool->setWorkspaceRoot(root);
    }

    // ── Set workspace root on new tools ───────────────────────────────────
    auto *delTool = dynamic_cast<DeleteFileTool *>(findTool(QStringLiteral("delete_file")));
    if (delTool) delTool->setWorkspaceRoot(root);

    auto *renameTool = dynamic_cast<RenameFileTool *>(findTool(QStringLiteral("rename_file")));
    if (renameTool) renameTool->setWorkspaceRoot(root);

    auto *copyTool = dynamic_cast<CopyFileTool *>(findTool(QStringLiteral("copy_file")));
    if (copyTool) copyTool->setWorkspaceRoot(root);

    auto *watchTool = dynamic_cast<FileWatcherTool *>(findTool(QStringLiteral("file_watcher")));
    if (watchTool) watchTool->setWorkspaceRoot(root);

    auto *diffTool = dynamic_cast<DiffTool *>(findTool(QStringLiteral("diff")));
    if (diffTool) diffTool->setWorkspaceRoot(root);

    auto *dbTool = dynamic_cast<DatabaseTool *>(findTool(QStringLiteral("database_query")));
    if (dbTool) dbTool->setWorkspaceRoot(root);

    // ── Set workspace root on sandbox & dev tools ─────────────────────────
    auto *hashTool = dynamic_cast<FileHashTool *>(findTool(QStringLiteral("file_content_hash")));
    if (hashTool) hashTool->setWorkspaceRoot(root);

    auto *wsConfigTool = dynamic_cast<WorkspaceConfigTool *>(findTool(QStringLiteral("workspace_config")));
    if (wsConfigTool) wsConfigTool->setWorkspaceRoot(root);

    auto *compileRunTool = dynamic_cast<CompileAndRunTool *>(findTool(QStringLiteral("compile_and_run")));
    if (compileRunTool) compileRunTool->setWorkspaceRoot(root);

    auto *archiveTool = dynamic_cast<ArchiveTool *>(findTool(QStringLiteral("archive")));
    if (archiveTool) archiveTool->setWorkspaceRoot(root);

    auto *patchTool = dynamic_cast<CreatePatchTool *>(findTool(QStringLiteral("create_patch_file")));
    if (patchTool) patchTool->setWorkspaceRoot(root);

    auto *imgTool = dynamic_cast<ImageInfoTool *>(findTool(QStringLiteral("image_info")));
    if (imgTool) imgTool->setWorkspaceRoot(root);

    auto *diagramTool = dynamic_cast<GenerateDiagramTool *>(findTool(QStringLiteral("generate_diagram")));
    if (diagramTool) diagramTool->setWorkspaceRoot(root);

    auto *brainDbTool = dynamic_cast<ProjectBrainDbTool *>(findTool(QStringLiteral("project_brain_db")));
    if (brainDbTool) brainDbTool->setWorkspaceRoot(root);

    auto *overwriteTool = dynamic_cast<OverwriteFileTool *>(findTool(QStringLiteral("write_file")));
    if (overwriteTool) overwriteTool->setWorkspaceRoot(root);

    auto *dockerTool = dynamic_cast<DockerTool *>(findTool(QStringLiteral("docker")));
    if (dockerTool) dockerTool->setWorkspaceRoot(root);
}

DiagnosticsNotifier *AgentPlatformBootstrap::diagnosticsNotifier() const
{
    return m_diagnosticsNotifier.get();
}