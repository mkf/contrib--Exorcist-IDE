#ifndef D5B507CA_8058_4248_8C39_B793CE7AFB48
#define D5B507CA_8058_4248_8C39_B793CE7AFB48

#include <functional>
#include <memory>
#include <vector>

#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QStringList>

#include "../aiinterface.h"
#include "../core/iprocess.h"
#include "agenttoolcallbacks.h"

class DiagnosticsNotifier;
class IFileSystem;
class PluginManager;
class ServiceRegistry;
class AgentOrchestrator;
class AgentController;
class AgentProviderRegistry;
class AgentRequestRouter;
class BrainContextBuilder;
class ChatSessionService;
class ContextBuilder;
class IAgentSettingsPageProvider;
class IChatSessionImporter;
class IProviderAuthIntegration;
class MemorySuggestionEngine;
class ProjectBrainService;
class SessionStore;
class TerminalSessionManager;
class ToolApprovalService;
class ToolRegistry;
class InProcessMcpServer;

class AgentUIBus;

class AgentPlatformBootstrap : public QObject
{
    Q_OBJECT

public:
    using Callbacks = AgentToolCallbacks;

    explicit AgentPlatformBootstrap(AgentOrchestrator *orchestrator,
                                    ServiceRegistry *services,
                                    IFileSystem *fileSystem,
                                    QObject *parent = nullptr);
    ~AgentPlatformBootstrap() override;

    void initialize(const Callbacks &callbacks);
    void registerCoreTools(const QString &workspaceRoot = {});
    void registerPluginProviders(PluginManager *pluginManager);
    void setWorkspaceRoot(const QString &root);

    AgentController *agentController() const { return m_agentController; }
    ContextBuilder  *contextBuilder() const { return m_contextBuilder; }
    SessionStore    *sessionStore() const { return m_sessionStore; }
    ToolRegistry    *toolRegistry() const { return m_toolRegistry; }

    /// In-process MCP servers hosting the non-core tools. The host registers
    /// these with its MCP client so their tools are discovered and adapted.
    QList<InProcessMcpServer *> mcpServers() const;

    AgentProviderRegistry *providerRegistry() const { return m_providerRegistry; }
    AgentRequestRouter    *requestRouter() const { return m_requestRouter; }
    ChatSessionService    *chatSessionService() const { return m_chatSessionService; }
    ToolApprovalService   *toolApprovalService() const { return m_toolApprovalService; }
    ProjectBrainService   *brainService() const { return m_brainService; }
    MemorySuggestionEngine *memorySuggestionEngine() const { return m_memorySuggestionEngine; }
    DiagnosticsNotifier    *diagnosticsNotifier() const;
    AgentUIBus             *uiBus() const { return m_uiBus; }

    // Plugin extension accessors (populated by registerPluginProviders)
    const QList<IChatSessionImporter *>      &sessionImporters() const { return m_sessionImporters; }
    const QList<IProviderAuthIntegration *>   &authIntegrations() const { return m_authIntegrations; }
    const QList<IAgentSettingsPageProvider *> &settingsPages() const { return m_settingsPages; }

private:
    /// Look up a tool by name across the direct registry and the hosted servers.
    ITool *findTool(const QString &name) const;

    AgentOrchestrator *m_orchestrator;
    ServiceRegistry   *m_services;
    IFileSystem       *m_fileSystem;
    Callbacks          m_callbacks;
    std::unique_ptr<IProcess> m_process;

    TerminalSessionManager *m_sessionManager = nullptr;
    ToolRegistry    *m_toolRegistry = nullptr;
    ContextBuilder  *m_contextBuilder = nullptr;
    AgentController *m_agentController = nullptr;
    SessionStore    *m_sessionStore = nullptr;

    AgentProviderRegistry *m_providerRegistry = nullptr;
    AgentRequestRouter    *m_requestRouter = nullptr;
    ChatSessionService    *m_chatSessionService = nullptr;
    ToolApprovalService   *m_toolApprovalService = nullptr;
    ProjectBrainService   *m_brainService = nullptr;
    BrainContextBuilder   *m_brainBuilder = nullptr;
    MemorySuggestionEngine *m_memorySuggestionEngine = nullptr;
    std::unique_ptr<DiagnosticsNotifier> m_diagnosticsNotifier;
    AgentUIBus *m_uiBus = nullptr;

    // In-process MCP servers hosting the non-core tools.
    std::vector<std::unique_ptr<InProcessMcpServer>> m_mcpServers;

    // Plugin extension registries
    QList<IChatSessionImporter *>      m_sessionImporters;
    QList<IProviderAuthIntegration *>  m_authIntegrations;
    QList<IAgentSettingsPageProvider *> m_settingsPages;
};


#endif /* D5B507CA_8058_4248_8C39_B793CE7AFB48 */
