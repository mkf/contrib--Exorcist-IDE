#ifndef EXORCIST_AGENTTOOLCALLBACKS_H
#define EXORCIST_AGENTTOOLCALLBACKS_H

#include <functional>

#include <QHash>
#include <QPixmap>
#include <QString>
#include <QStringList>

#include "../aiinterface.h"
#include "tools/askusertool.h"
#include "tools/buildtools.h"
#include "tools/changeimpacttool.h"
#include "tools/devtools.h"
#include "tools/difftool.h"
#include "tools/editorcontexttool.h"
#include "tools/formatcodetool.h"
#include "tools/lsptools.h"
#include "tools/luatool.h"
#include "tools/refactortool.h"
#include "tools/staticanalysistool.h"
#include "tools/treesitterquerytool.h"

// Host-provided collaborators for the agent tool families. The platform
// constructs these once; the in-process MCP servers consume them.
struct AgentToolCallbacks
{
    std::function<QStringList()> openFilesGetter;
    std::function<QString()> gitStatusGetter;
    std::function<QString()> terminalOutputGetter;
    std::function<QList<AgentDiagnostic>()> diagnosticsGetter;
    std::function<QHash<QString, QString>()> changedFilesGetter;
    std::function<QString(const QString &)> gitDiffGetter;

    // Debug adapter callbacks
    std::function<QString(const QString &, int, const QString &)> debugBreakpointSetter;
    std::function<bool(const QString &, int)> debugBreakpointRemover;
    std::function<QString(int)> debugStackGetter;
    std::function<QString(int)> debugVariablesGetter;
    std::function<QString(const QString &, int)> debugEvaluator;
    std::function<bool(const QString &)> debugStepper;

    // Screenshot
    std::function<QPixmap(const QString &)> widgetGrabber;

    // Introspection
    std::function<QString(const QString &)> introspectionHandler;

    // Lua execution
    LuaExecuteTool::LuaExecutor luaExecutor;

    // Code graph / intelligence
    std::function<QString(const QString &, int)>              symbolSearchFn;
    std::function<QString(const QString &)>                   symbolsInFileFn;
    std::function<QString(const QString &, int, int)>         findReferencesFn;
    std::function<QString(const QString &, int, int)>         findDefinitionFn;
    std::function<QString(const QString &, int)>              chunkSearchFn;

    // Build & test
    BuildProjectTool::Builder       buildProjectFn;
    RunTestsTool::TestRunner         runTestsFn;
    std::function<QStringList()>     buildTargetsGetter;

    // Code formatting
    FormatCodeTool::CodeFormatter   codeFormatter;

    // Refactoring (LSP)
    RefactorTool::Refactorer        refactorer;

    // User interaction
    AskUserTool::UserPrompter       userPrompter;

    // Editor context
    EditorContextTool::EditorStateGetter editorStateGetter;

    // Change impact analysis
    ChangeImpactTool::ImpactAnalyzer    changeImpactAnalyzer;

    // Navigation
    std::function<bool(const QString &, int, int)>           fileOpener;
    std::function<QString(const QString &)>                   headerSourceSwitcher;

    // LSP rename & usages
    RenameSymbolTool::SymbolRenamer     symbolRenamer;
    ListCodeUsagesTool::UsageFinder     usageFinder;

    // Diff visualization
    DiffTool::DiffViewer                diffViewer;

    // Static analysis
    StaticAnalysisTool::StaticAnalyzer  staticAnalyzer;

    // Terminal selection
    std::function<QString()>            terminalSelectionGetter;

    // Test failure cache
    std::function<RunTestsTool::TestResult()> testFailureGetter;

    // IDE command execution
    std::function<bool(const QString &)> commandExecutor;
    std::function<QStringList()>         commandListGetter;

    // Tree-sitter AST parsing
    TreeSitterParseTool::TreeSitterParser treeSitterParser;

    // Tree-sitter rich AST query
    TreeSitterQueryTool::FileParserFn      tsFileParser;
    TreeSitterQueryTool::QueryRunnerFn     tsQueryRunner;
    TreeSitterQueryTool::SymbolExtractorFn tsSymbolExtractor;
    TreeSitterQueryTool::NodeAtPositionFn  tsNodeAtPosition;

    // Diagram generation (Mermaid/PlantUML)
    GenerateDiagramTool::DiagramRenderer diagramRenderer;

    // Performance profiling
    PerformanceProfileTool::Profiler     profiler;

    // Symbol documentation (LSP hover)
    SymbolDocTool::DocGetter             symbolDocGetter;

    // Code completion (LSP)
    CodeCompletionTool::CompletionGetter completionGetter;

    // Secure key storage
    std::function<bool(const QString &, const QString &)> secureKeyStorer;
    std::function<QString(const QString &)>               secureKeyGetter;
    std::function<QStringList()>                           secureKeyLister;
    std::function<bool(const QString &)>                  secureKeyDeleter;
};

#endif // EXORCIST_AGENTTOOLCALLBACKS_H