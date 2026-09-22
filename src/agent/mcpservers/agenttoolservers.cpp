#include "agenttoolservers.h"

#include "unavailabletool.h"
#include "../agentcontroller.h"
#include "../agentorchestrator.h"
#include "../agenttoolcallbacks.h"
#include "../contextbuilder.h"
#include "../itool.h"
#include "../projectbrainservice.h"
#include "../terminalsessionmanager.h"
#include "../ui/agentuibus.h"
#include "../tools/advancedtools.h"
#include "../tools/agentmemorydb.h"
#include "../tools/applypatchtool.h"
#include "../tools/askusertool.h"
#include "../tools/buildtools.h"
#include "../tools/changeimpacttool.h"
#include "../tools/clipboardtool.h"
#include "../tools/codegraphtool.h"
#include "../tools/dashboardtools.h"
#include "../tools/databasetool.h"
#include "../tools/debugtools.h"
#include "../tools/devtools.h"
#include "../tools/difftool.h"
#include "../tools/dockertools.h"
#include "../tools/editorcontexttool.h"
#include "../tools/filemanagementtools.h"
#include "../tools/filesystemtools.h"
#include "../tools/formatcodetool.h"
#include "../tools/githubmcptools.h"
#include "../tools/httptool.h"
#include "../tools/idecommandtool.h"
#include "../tools/idetools.h"
#include "../tools/introspecttool.h"
#include "../tools/lsptools.h"
#include "../tools/luaagenttoolstore.h"
#include "../tools/luatool.h"
#include "../tools/managememorytool.h"
#include "../tools/managerulestool.h"
#include "../tools/memorytool.h"
#include "../tools/moretools.h"
#include "../tools/navigationtools.h"
#include "../tools/notebooktools.h"
#include "../tools/packagemanagertools.h"
#include "../tools/projectbraindb.h"
#include "../tools/projectdbtool.h"
#include "../tools/projectinfotool.h"
#include "../tools/pythontools.h"
#include "../tools/refactortool.h"
#include "../tools/runcommandtool.h"
#include "../tools/sandboxtools.h"
#include "../tools/screenshottool.h"
#include "../tools/scratchpadtool.h"
#include "../tools/securitytools.h"
#include "../tools/staticanalysistool.h"
#include "../tools/subagenttool.h"
#include "../tools/systemtools.h"
#include "../tools/terminalsessiontools.h"
#include "../tools/transactiontool.h"
#include "../tools/treesitterquerytool.h"
#include "../tools/websearchtool.h"
#include "../../core/ifilesystem.h"
#include "../../core/iprocess.h"

#include <QFile>

#include <utility>
#include <vector>

// ── TerminalToolServer ────────────────────────────────────────────────────────

TerminalToolServer::TerminalToolServer(IProcess *process,
                                       const QString &workspaceRoot,
                                       TerminalSessionManager *sessionManager,
                                       const AgentToolCallbacks &callbacks)
    : InProcessMcpServer(QStringLiteral("terminal"))
{
    addTool(std::make_unique<RunCommandTool>(process, workspaceRoot, sessionManager));
    addTool(std::make_unique<GetTerminalOutputTool>(sessionManager));
    addTool(std::make_unique<KillTerminalTool>(sessionManager));
    addTool(std::make_unique<AwaitTerminalTool>(sessionManager));
    if (callbacks.terminalSelectionGetter)
        addTool(std::make_unique<TerminalSelectionTool>(callbacks.terminalSelectionGetter));
    if (callbacks.terminalOutputGetter)
        addTool(std::make_unique<TerminalLastCommandTool>(callbacks.terminalOutputGetter));
}

// ── FileManagementToolServer ──────────────────────────────────────────────────

FileManagementToolServer::FileManagementToolServer(
    IFileSystem *fileSystem,
    const QString &workspaceRoot,
    AgentController *controller,
    std::shared_ptr<TransactionStore> txStore)
    : InProcessMcpServer(QStringLiteral("file-management"))
{
    {
        auto overwriteTool = std::make_unique<OverwriteFileTool>(fileSystem);
        overwriteTool->setTransactionStore(txStore);
        addTool(std::move(overwriteTool));
    }
    {
        auto snapshotGetter = [controller]() -> QHash<QString, QString> {
            auto *session = controller ? controller->session() : nullptr;
            return session ? session->fileSnapshots() : QHash<QString, QString>{};
        };
        auto fileRestorer = [controller, fileSystem](const QString &path)
            -> UndoFileEditTool::UndoResult {
            auto *session = controller ? controller->session() : nullptr;
            if (!session)
                return {false, 0, QStringLiteral("No active session.")};
            const auto snaps = session->fileSnapshots();
            if (!snaps.contains(path))
                return {false, 0, QStringLiteral("No snapshot for: %1").arg(path)};

            const QString &original = snaps[path];
            if (original.isNull()) {
                // File didn't exist before — delete it
                QFile::remove(path);
                return {true, 1, QStringLiteral("Deleted (file did not exist before).")};
            }
            QString error;
            if (!fileSystem->writeTextFile(path, original, &error))
                return {false, 0, error};
            return {true, 1, QStringLiteral("Restored to pre-edit state.")};
        };
        addTool(std::make_unique<UndoFileEditTool>(
            std::move(snapshotGetter), std::move(fileRestorer)));
    }
    addTool(std::make_unique<InsertEditIntoFileTool>());
    addTool(std::make_unique<ReadProjectStructureTool>(workspaceRoot));
    addTool(std::make_unique<DeleteFileTool>());
    addTool(std::make_unique<RenameFileTool>());
    addTool(std::make_unique<CopyFileTool>());
    addTool(std::make_unique<FileWatcherTool>());
}

// ── OrchestrationToolServer ───────────────────────────────────────────────────

OrchestrationToolServer::OrchestrationToolServer(
    AgentOrchestrator *orchestrator,
    ToolRegistry *toolRegistry,
    ContextBuilder *contextBuilder,
    const QString &appDataPath)
    : InProcessMcpServer(QStringLiteral("orchestration"))
{
    addTool(std::make_unique<ManageTodoListTool>(
        appDataPath + QStringLiteral("/agent_todo.json")));
    addTool(std::make_unique<SubagentTool>(orchestrator, toolRegistry, contextBuilder));
}

// ── MemoryToolServer ──────────────────────────────────────────────────────────

MemoryToolServer::MemoryToolServer(ProjectBrainService *brainService,
                                   const QString &appDataPath)
    : InProcessMcpServer(QStringLiteral("memory"))
{
    addTool(std::make_unique<MemoryTool>(appDataPath + QStringLiteral("/memories")));
    addTool(std::make_unique<ScratchpadTool>(appDataPath + QStringLiteral("/scratchpad")));
    addTool(std::make_unique<AgentMemoryDbTool>());
    addTool(std::make_unique<ProjectBrainDbTool>());
    addTool(std::make_unique<ManageRulesTool>(brainService));
    addTool(std::make_unique<ManageMemoryTool>(brainService));
}

// ── CodeIntelligenceToolServer ────────────────────────────────────────────────

CodeIntelligenceToolServer::CodeIntelligenceToolServer(const AgentToolCallbacks &cb)
    : InProcessMcpServer(QStringLiteral("code-intelligence"))
{
    addTool(std::make_unique<GetErrorsTool>(cb.diagnosticsGetter));
    if (cb.symbolSearchFn) {
        addTool(std::make_unique<CodeGraphTool>(
            cb.symbolSearchFn, cb.symbolsInFileFn, cb.findReferencesFn,
            cb.findDefinitionFn, cb.chunkSearchFn));
    }
    if (cb.codeFormatter)
        addTool(std::make_unique<FormatCodeTool>(cb.codeFormatter));
    if (cb.refactorer)
        addTool(std::make_unique<RefactorTool>(cb.refactorer));
    if (cb.changeImpactAnalyzer)
        addTool(std::make_unique<ChangeImpactTool>(cb.changeImpactAnalyzer));
    if (cb.symbolRenamer)
        addTool(std::make_unique<RenameSymbolTool>(cb.symbolRenamer));
    if (cb.usageFinder)
        addTool(std::make_unique<ListCodeUsagesTool>(cb.usageFinder));
    if (cb.staticAnalyzer)
        addTool(std::make_unique<StaticAnalysisTool>(cb.staticAnalyzer));
    if (cb.treeSitterParser)
        addTool(std::make_unique<TreeSitterParseTool>(cb.treeSitterParser));
    if (cb.tsFileParser && cb.tsQueryRunner && cb.tsSymbolExtractor && cb.tsNodeAtPosition) {
        addTool(std::make_unique<TreeSitterQueryTool>(
            cb.tsFileParser, cb.tsQueryRunner, cb.tsSymbolExtractor, cb.tsNodeAtPosition));
    }
    if (cb.diagramRenderer)
        addTool(std::make_unique<GenerateDiagramTool>(cb.diagramRenderer));
    if (cb.profiler)
        addTool(std::make_unique<PerformanceProfileTool>(cb.profiler));
    if (cb.symbolDocGetter)
        addTool(std::make_unique<SymbolDocTool>(cb.symbolDocGetter));
    if (cb.completionGetter)
        addTool(std::make_unique<CodeCompletionTool>(cb.completionGetter));
}

// ── DebugToolServer ───────────────────────────────────────────────────────────

DebugToolServer::DebugToolServer(const AgentToolCallbacks &cb)
    : InProcessMcpServer(QStringLiteral("debug"))
{
    if (cb.debugBreakpointSetter) {
        addTool(std::make_unique<DebugSetBreakpointTool>(
            cb.debugBreakpointSetter, cb.debugBreakpointRemover));
    }
    if (cb.debugStackGetter)
        addTool(std::make_unique<DebugGetStackTraceTool>(cb.debugStackGetter));
    if (cb.debugVariablesGetter) {
        addTool(std::make_unique<DebugGetVariablesTool>(
            cb.debugVariablesGetter, cb.debugEvaluator));
    }
    if (cb.debugStepper)
        addTool(std::make_unique<DebugStepTool>(cb.debugStepper));
}

// ── EditorToolServer ──────────────────────────────────────────────────────────

EditorToolServer::EditorToolServer(const AgentToolCallbacks &cb,
                                   const QString &workspaceRoot)
    : InProcessMcpServer(QStringLiteral("editor"))
{
    if (cb.fileOpener)
        addTool(std::make_unique<OpenFileTool>(cb.fileOpener));
    addTool(std::make_unique<SwitchHeaderSourceTool>(cb.headerSourceSwitcher));
    if (cb.editorStateGetter)
        addTool(std::make_unique<EditorContextTool>(cb.editorStateGetter));
    addTool(std::make_unique<ClipboardTool>());
    {
        auto diffTool = std::make_unique<DiffTool>(cb.diffViewer);
        diffTool->setWorkspaceRoot(workspaceRoot);
        addTool(std::move(diffTool));
    }
}

// ── IdeCommandToolServer ──────────────────────────────────────────────────────

IdeCommandToolServer::IdeCommandToolServer(const AgentToolCallbacks &cb)
    : InProcessMcpServer(QStringLiteral("ide-commands"))
{
    if (cb.userPrompter)
        addTool(std::make_unique<AskUserTool>(cb.userPrompter));
    if (cb.widgetGrabber)
        addTool(std::make_unique<ScreenshotTool>(cb.widgetGrabber));
    if (cb.introspectionHandler)
        addTool(std::make_unique<IntrospectTool>(cb.introspectionHandler));
    if (cb.commandExecutor) {
        addTool(std::make_unique<RunIdeCommandTool>(
            cb.commandExecutor, cb.commandListGetter));
    }
}

// ── BuildTestToolServer ───────────────────────────────────────────────────────

BuildTestToolServer::BuildTestToolServer(const AgentToolCallbacks &cb,
                                         IFileSystem *fileSystem,
                                         std::shared_ptr<TransactionStore> txStore)
    : InProcessMcpServer(QStringLiteral("build-test"))
{
    addTool(std::make_unique<BeginTransactionTool>(txStore));
    addTool(std::make_unique<CommitTransactionTool>(txStore));
    addTool(std::make_unique<RollbackTransactionTool>(txStore));
    addTool(std::make_unique<ApplyPatchTool>(fileSystem));
    if (cb.buildProjectFn)
        addTool(std::make_unique<BuildProjectTool>(cb.buildProjectFn));
    if (cb.runTestsFn)
        addTool(std::make_unique<RunTestsTool>(cb.runTestsFn));
    if (cb.buildTargetsGetter)
        addTool(std::make_unique<GetBuildTargetsTool>(cb.buildTargetsGetter));
    if (cb.testFailureGetter)
        addTool(std::make_unique<TestFailureTool>(cb.testFailureGetter));
    addTool(std::make_unique<CompileAndRunTool>());
    addTool(std::make_unique<ArchiveTool>());
    addTool(std::make_unique<CreatePatchTool>());
}

// ── WorkspaceToolServer ───────────────────────────────────────────────────────

WorkspaceToolServer::WorkspaceToolServer(const QString &workspaceRoot,
                                         IFileSystem *fileSystem)
    : InProcessMcpServer(QStringLiteral("workspace"))
{
    addTool(std::make_unique<GetProjectSetupInfoTool>(workspaceRoot));
    addTool(std::make_unique<WorkspaceConfigTool>(fileSystem));
}

// ── GitToolServer ─────────────────────────────────────────────────────────────

GitToolServer::GitToolServer(const AgentToolCallbacks &cb)
    : InProcessMcpServer(QStringLiteral("git"))
{
    addTool(std::make_unique<GitStatusTool>(cb.gitStatusGetter));
    addTool(std::make_unique<GitDiffTool>(cb.gitDiffGetter));
}

// ── NotebookToolServer ────────────────────────────────────────────────────────

NotebookToolServer::NotebookToolServer(QObject *parent)
    : InProcessMcpServer(QStringLiteral("notebook"))
{
    auto *nbMgr = new NotebookManager(parent);
    addTool(std::make_unique<NotebookContextTool>(nbMgr));
    addTool(std::make_unique<ReadCellOutputTool>(nbMgr));
    addTool(std::make_unique<CreateNotebookTool>());
    addTool(std::make_unique<EditNotebookCellsTool>());
    addTool(std::make_unique<GetNotebookSummaryTool>());
}

// ── PythonToolServer ──────────────────────────────────────────────────────────

PythonToolServer::PythonToolServer(TerminalSessionManager *sessionManager)
    : InProcessMcpServer(QStringLiteral("python"))
{
    addTool(std::make_unique<PythonEnvTool>(sessionManager));
    addTool(std::make_unique<InstallPythonPackagesTool>(sessionManager));
    addTool(std::make_unique<RunPythonTool>(sessionManager));
}

// ── NodeToolServer ────────────────────────────────────────────────────────────

NodeToolServer::NodeToolServer(TerminalSessionManager *sessionManager)
    : InProcessMcpServer(QStringLiteral("node"))
{
    addTool(std::make_unique<PackageJsonInfoTool>());
    addTool(std::make_unique<NpmRunTool>(sessionManager));
    addTool(std::make_unique<InstallNodePackagesTool>(sessionManager));
}

// ── DockerToolServer ──────────────────────────────────────────────────────────

DockerToolServer::DockerToolServer(IProcess *process)
    : InProcessMcpServer(QStringLiteral("docker"))
{
    addTool(std::make_unique<DockerTool>(process));
}

// ── LuaToolServer ─────────────────────────────────────────────────────────────

LuaToolServer::LuaToolServer(const AgentToolCallbacks &cb)
    : InProcessMcpServer(QStringLiteral("lua"))
{
    if (cb.luaExecutor)
        addTool(std::make_unique<LuaExecuteTool>(cb.luaExecutor));
    addTool(std::make_unique<SaveLuaToolTool>());
    addTool(std::make_unique<ListLuaToolsTool>());
    if (cb.luaExecutor)
        addTool(std::make_unique<RunLuaToolTool>(cb.luaExecutor));
}

// ── DashboardToolServer ───────────────────────────────────────────────────────

DashboardToolServer::DashboardToolServer(AgentUIBus *uiBus)
    : InProcessMcpServer(QStringLiteral("dashboard"))
{
    addTool(std::make_unique<CreateDashboardMissionTool>(uiBus));
    addTool(std::make_unique<UpdateDashboardStepTool>(uiBus));
    addTool(std::make_unique<UpdateDashboardMetricTool>(uiBus));
    addTool(std::make_unique<AddDashboardLogTool>(uiBus));
    addTool(std::make_unique<AddDashboardArtifactTool>(uiBus));
    addTool(std::make_unique<CompleteDashboardMissionTool>(uiBus));
}

// ── SecretsToolServer ─────────────────────────────────────────────────────────

SecretsToolServer::SecretsToolServer(const AgentToolCallbacks &cb)
    : InProcessMcpServer(QStringLiteral("secrets"))
{
    if (cb.secureKeyStorer && cb.secureKeyGetter) {
        addTool(std::make_unique<StoreSecretTool>(cb.secureKeyStorer));
        addTool(std::make_unique<GetSecretTool>(cb.secureKeyGetter));
        addTool(std::make_unique<ListSecretsTool>(cb.secureKeyLister));
        if (cb.secureKeyDeleter)
            addTool(std::make_unique<DeleteSecretTool>(cb.secureKeyDeleter));
    }
}