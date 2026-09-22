#include "agenttoolservers.h"

#include "unavailabletool.h"
#include "../tools/databasetool.h"
#include "../tools/devtools.h"
#include "../tools/githubmcptools.h"
#include "../tools/httptool.h"
#include "../tools/moretools.h"
#include "../tools/projectdbtool.h"
#include "../tools/sandboxtools.h"
#include "../tools/systemtools.h"
#include "../tools/websearchtool.h"

#include <utility>
#include <vector>

// ── Servers with no host collaborators ────────────────────────────────────────
//
// These servers are self-contained: their tools need no platform services, so
// they can be constructed and tested in isolation.

// ── GithubToolServer ──────────────────────────────────────────────────────────

GithubToolServer::GithubToolServer()
    : InProcessMcpServer(QStringLiteral("github"))
{
    addTool(std::make_unique<GitHubIssuesTool>());
    addTool(std::make_unique<GitHubPRTool>());
    addTool(std::make_unique<GitHubCodeSearchTool>());
    addTool(std::make_unique<GitHubRepoInfoTool>());
}

// ── WebToolServer ─────────────────────────────────────────────────────────────

WebToolServer::WebToolServer()
    : InProcessMcpServer(QStringLiteral("web"))
{
    addTool(std::make_unique<FetchWebpageTool>());
    addTool(std::make_unique<WebSearchTool>());
    addTool(std::make_unique<HttpRequestTool>());
}

// ── BrowserToolServer ─────────────────────────────────────────────────────────

BrowserToolServer::BrowserToolServer()
    : InProcessMcpServer(QStringLiteral("browser"))
{
    const std::vector<std::pair<QString, QString>> tools = {
        {QStringLiteral("navigate"), QStringLiteral("Navigate the browser to a URL.")},
        {QStringLiteral("navigate_back"), QStringLiteral("Go back in the browser history.")},
        {QStringLiteral("snapshot"), QStringLiteral("Return an accessibility snapshot of the page.")},
        {QStringLiteral("take_screenshot"), QStringLiteral("Capture a page screenshot.")},
        {QStringLiteral("click"), QStringLiteral("Click an element by reference.")},
        {QStringLiteral("hover"), QStringLiteral("Hover over an element.")},
        {QStringLiteral("type"), QStringLiteral("Type text into the page.")},
        {QStringLiteral("fill_form"), QStringLiteral("Fill a form field.")},
        {QStringLiteral("select_option"), QStringLiteral("Select a dropdown option.")},
        {QStringLiteral("press_key"), QStringLiteral("Press a keyboard key.")},
        {QStringLiteral("drag"), QStringLiteral("Drag and drop an element.")},
        {QStringLiteral("file_upload"), QStringLiteral("Upload files to the page.")},
        {QStringLiteral("handle_dialog"), QStringLiteral("Accept or dismiss a modal dialog.")},
        {QStringLiteral("evaluate"), QStringLiteral("Evaluate JavaScript in the page.")},
        {QStringLiteral("run_code"), QStringLiteral("Run Playwright code against the page.")},
        {QStringLiteral("console_messages"), QStringLiteral("Read browser console messages.")},
        {QStringLiteral("network_requests"), QStringLiteral("List network requests.")},
        {QStringLiteral("tabs"), QStringLiteral("Manage browser tabs.")},
        {QStringLiteral("resize"), QStringLiteral("Resize the browser viewport.")},
        {QStringLiteral("wait_for"), QStringLiteral("Wait for text or a duration.")},
        {QStringLiteral("close"), QStringLiteral("Close the browser page.")},
        {QStringLiteral("install"), QStringLiteral("Install the browser runtime.")},
    };
    for (const auto &entry : tools)
        addTool(std::make_unique<UnavailableTool>(entry.first, entry.second));
}

// ── PylanceToolServer ─────────────────────────────────────────────────────────

PylanceToolServer::PylanceToolServer()
    : InProcessMcpServer(QStringLiteral("pylance"))
{
    const std::vector<std::pair<QString, QString>> tools = {
        {QStringLiteral("pylance_documents"), QStringLiteral("Search Pylance documentation.")},
        {QStringLiteral("pylance_settings"), QStringLiteral("Read current Python analysis settings.")},
        {QStringLiteral("pylance_python_environments"), QStringLiteral("List Python environments.")},
        {QStringLiteral("pylance_update_python_environment"), QStringLiteral("Switch the active Python environment.")},
        {QStringLiteral("pylance_workspace_roots"), QStringLiteral("Get workspace roots.")},
        {QStringLiteral("pylance_workspace_user_files"), QStringLiteral("List user Python files.")},
        {QStringLiteral("pylance_imports"), QStringLiteral("Analyze workspace imports.")},
        {QStringLiteral("pylance_installed_top_level_modules"), QStringLiteral("List installed top-level modules.")},
        {QStringLiteral("pylance_syntax_errors"), QStringLiteral("Check Python code for syntax errors.")},
        {QStringLiteral("pylance_file_syntax_errors"), QStringLiteral("Check a Python file for syntax errors.")},
        {QStringLiteral("pylance_invoke_refactoring"), QStringLiteral("Invoke a Pylance refactoring.")},
        {QStringLiteral("pylance_run_code_snippet"), QStringLiteral("Run a Python code snippet.")},
    };
    for (const auto &entry : tools)
        addTool(std::make_unique<UnavailableTool>(entry.first, entry.second));
}

// ── SonarQubeToolServer ───────────────────────────────────────────────────────

SonarQubeToolServer::SonarQubeToolServer()
    : InProcessMcpServer(QStringLiteral("sonarqube"))
{
    addTool(std::make_unique<UnavailableTool>(
        QStringLiteral("analyze_file"), QStringLiteral("Analyze a file with SonarQube.")));
    addTool(std::make_unique<UnavailableTool>(
        QStringLiteral("list_potential_security_issues"), QStringLiteral("List potential security issues.")));
    addTool(std::make_unique<UnavailableTool>(
        QStringLiteral("exclude_from_analysis"), QStringLiteral("Exclude files from analysis.")));
    addTool(std::make_unique<UnavailableTool>(
        QStringLiteral("setup_connected_mode"), QStringLiteral("Set up SonarQube Connected Mode.")));
}

// ── DatabaseToolServer ────────────────────────────────────────────────────────

DatabaseToolServer::DatabaseToolServer()
    : InProcessMcpServer(QStringLiteral("database"))
{
    addTool(std::make_unique<DatabaseTool>());
    addTool(std::make_unique<ProjectDatabaseTool>());
}

// ── SystemToolServer ──────────────────────────────────────────────────────────

SystemToolServer::SystemToolServer()
    : InProcessMcpServer(QStringLiteral("system"))
{
    addTool(std::make_unique<ProcessManagementTool>());
    addTool(std::make_unique<NetworkTool>());
    addTool(std::make_unique<JsonParseFormatTool>());
    addTool(std::make_unique<CurrentTimeTool>());
    addTool(std::make_unique<RegexTestTool>());
    addTool(std::make_unique<EnvironmentVariablesTool>());
    addTool(std::make_unique<FileHashTool>());
    addTool(std::make_unique<ImageInfoTool>());
}