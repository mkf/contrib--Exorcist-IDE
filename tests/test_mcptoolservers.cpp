#include <QTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>

#include "agent/itool.h"
#include "mcp/inprocessmcpserver.h"
#include "mcp/mcpclient.h"
#include "mcp/mcptooladapter.h"
#include "mcp/tooldiscoverybridge.h"

// ── Fake tool ─────────────────────────────────────────────────────────────────

class FakeTool : public ITool
{
public:
    FakeTool(const QString &name, AgentToolPermission perm,
             const QString &text = QStringLiteral("ok"))
        : m_name(name), m_perm(perm), m_text(text)
    {
        m_spec.name        = name;
        m_spec.description = name + QStringLiteral(" description");
        m_spec.permission  = perm;
        m_spec.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("path"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("string")}}},
            }},
        };
    }

    ToolSpec spec() const override { return m_spec; }

    ToolExecResult invoke(const QJsonObject &args) override
    {
        ++m_invokeCount;
        m_lastArgs = args;
        return {true, {}, m_text, {}};
    }

    int invokeCount() const { return m_invokeCount; }
    QJsonObject lastArgs() const { return m_lastArgs; }

private:
    QString m_name;
    AgentToolPermission m_perm;
    QString m_text;
    ToolSpec m_spec;
    int m_invokeCount = 0;
    QJsonObject m_lastArgs;
};

// ── Tests ─────────────────────────────────────────────────────────────────────

class TestMcpToolServers : public QObject
{
    Q_OBJECT

private slots:

    // ── 2.2 Name normalization ────────────────────────────────────────────

    void normalizeStripsTransportPrefix()
    {
        QCOMPARE(InProcessMcpServer::normalizeToolName(
                     QStringLiteral("mcp_copilot_conta_list_containers")),
                 QStringLiteral("list_containers"));
        QCOMPARE(InProcessMcpServer::normalizeToolName(
                     QStringLiteral("mcp_mcp_docker_browser_navigate")),
                 QStringLiteral("navigate"));
    }

    void normalizeLeavesCleanNamesUntouched()
    {
        QCOMPARE(InProcessMcpServer::normalizeToolName(QStringLiteral("run_in_terminal")),
                 QStringLiteral("run_in_terminal"));
    }

    void addToolRejectsDuplicateNormalizedName()
    {
        InProcessMcpServer server(QStringLiteral("docker"));
        QVERIFY(server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("mcp_copilot_conta_list_containers"),
            AgentToolPermission::ReadOnly)));
        QVERIFY(!server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("mcp_copilot_conta_list_containers"),
            AgentToolPermission::ReadOnly)));
        QCOMPARE(server.toolNames().size(), 1);
    }

    // ── 1.1 / 1.2 tools/list + tools/call ─────────────────────────────────

    void listToolsMapsSpec()
    {
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous));

        const QJsonArray tools = server.listTools();
        QCOMPARE(tools.size(), 1);
        const QJsonObject def = tools.first().toObject();
        QCOMPARE(def[QStringLiteral("name")].toString(), QStringLiteral("run_in_terminal"));
        QVERIFY(!def[QStringLiteral("description")].toString().isEmpty());
        QVERIFY(def.contains(QStringLiteral("inputSchema")));
        QCOMPARE(def[QStringLiteral("x-permission")].toInt(),
                 static_cast<int>(AgentToolPermission::Dangerous));
    }

    void callToolDispatchesByName()
    {
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous,
            QStringLiteral("ran")));

        const QJsonObject result = server.callTool(
            QStringLiteral("run_in_terminal"), QJsonObject{});
        QVERIFY(!result[QStringLiteral("isError")].toBool());
        const QJsonArray content = result[QStringLiteral("content")].toArray();
        QCOMPARE(content.first().toObject()[QStringLiteral("text")].toString(),
                 QStringLiteral("ran"));
    }

    void callToolUnknownNameIsError()
    {
        InProcessMcpServer server(QStringLiteral("terminal"));
        const QJsonObject result = server.callTool(
            QStringLiteral("nope"), QJsonObject{});
        QVERIFY(result[QStringLiteral("isError")].toBool());
    }

    // ── 1.6 tools/list has no side effects ────────────────────────────────

    void listToolsDoesNotInvokeHandlers()
    {
        InProcessMcpServer server(QStringLiteral("terminal"));
        auto tool = std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous);
        FakeTool *raw = tool.get();
        server.addTool(std::move(tool));

        server.listTools();
        QCOMPARE(raw->invokeCount(), 0);
    }

    // ── 1.3 In-process transport ──────────────────────────────────────────

    void clientDiscoversInProcessServer()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous));

        QSignalSpy spy(&client, &McpClient::toolsDiscovered);
        client.addInProcessServer(&server);

        QCOMPARE(spy.count(), 1);
        QVERIFY(client.isConnected(QStringLiteral("terminal")));
        QVERIFY(client.isInProcess(QStringLiteral("terminal")));
        QCOMPARE(client.allTools().size(), 1);
        QCOMPARE(client.allTools().first().name, QStringLiteral("run_in_terminal"));
    }

    void clientCallToolRoutesInProcess()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous,
            QStringLiteral("ran")));
        client.addInProcessServer(&server);

        QSignalSpy spy(&client, &McpClient::toolCallFinished);
        client.callTool(QStringLiteral("run_in_terminal"), QJsonObject{},
                        QStringLiteral("req-1"));

        QCOMPARE(spy.count(), 1);
        const auto args = spy.takeFirst();
        QCOMPARE(args.at(0).toString(), QStringLiteral("req-1"));
        const McpToolResult res = args.at(1).value<McpToolResult>();
        QVERIFY(res.ok);
        QCOMPARE(res.text, QStringLiteral("ran"));
    }

    void removeInProcessServerStopsTools()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous));
        client.addInProcessServer(&server);
        QCOMPARE(client.allTools().size(), 1);

        client.removeInProcessServer(QStringLiteral("terminal"));
        QCOMPARE(client.allTools().size(), 0);
        QVERIFY(!client.isConnected(QStringLiteral("terminal")));
    }

    // ── 2.1 / 2.3 Namespacing + permission ────────────────────────────────

    void adapterNamespacesToolName()
    {
        McpClient client;
        McpToolInfo info;
        info.name = QStringLiteral("run_in_terminal");
        info.serverName = QStringLiteral("terminal");
        McpToolAdapter adapter(&client, info);

        QCOMPARE(adapter.spec().name, QStringLiteral("terminal__run_in_terminal"));
        QCOMPARE(McpToolAdapter::namespacedName(QStringLiteral("terminal"),
                                                QStringLiteral("run_in_terminal")),
                 QStringLiteral("terminal__run_in_terminal"));
        QCOMPARE(adapter.serverName(), QStringLiteral("terminal"));
        QCOMPARE(adapter.serverLocalName(), QStringLiteral("run_in_terminal"));
    }

    void adapterPreservesPermission()
    {
        McpClient client;
        McpToolInfo info;
        info.name = QStringLiteral("run_in_terminal");
        info.serverName = QStringLiteral("terminal");
        info.permission = AgentToolPermission::Dangerous;
        McpToolAdapter dangerous(&client, info);
        QCOMPARE(dangerous.spec().permission, AgentToolPermission::Dangerous);

        info.permission = AgentToolPermission::ReadOnly;
        McpToolAdapter safe(&client, info);
        QCOMPARE(safe.spec().permission, AgentToolPermission::ReadOnly);
    }

    void adapterInvokesThroughClient()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous,
            QStringLiteral("ran")));
        client.addInProcessServer(&server);

        McpToolInfo info;
        info.name = QStringLiteral("run_in_terminal");
        info.serverName = QStringLiteral("terminal");
        info.permission = AgentToolPermission::Dangerous;
        McpToolAdapter adapter(&client, info);

        const ToolExecResult res = adapter.invoke(QJsonObject{});
        QVERIFY(res.ok);
        QCOMPARE(res.textContent, QStringLiteral("ran"));
    }

    // ── 2.4 / 2.6 Progressive discovery ───────────────────────────────────

    void bridgeInactiveBelowThreshold()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous));
        client.addInProcessServer(&server);

        ToolDiscoveryBridge bridge(&client, 40);
        QVERIFY(!bridge.isActive());
        QVERIFY(bridge.createBridgeTools().empty());

        // No index is built below the threshold.
        bridge.search(QStringLiteral("terminal"));
        QVERIFY(!bridge.indexBuilt());
    }

    void bridgeActiveAboveThreshold()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous));
        client.addInProcessServer(&server);

        ToolDiscoveryBridge bridge(&client, 0);
        QVERIFY(bridge.isActive());
        QCOMPARE(bridge.createBridgeTools().size(), 3);

        const QJsonArray results = bridge.search(QStringLiteral("terminal"));
        QCOMPARE(results.size(), 1);
        QCOMPARE(results.first().toObject()[QStringLiteral("name")].toString(),
                 QStringLiteral("terminal__run_in_terminal"));
        QVERIFY(bridge.indexBuilt());

        const QJsonObject details = bridge.details(
            QStringLiteral("terminal__run_in_terminal"));
        QVERIFY(details.contains(QStringLiteral("inputSchema")));
    }

    void bridgeRejectsOutOfCatalogueInvocation()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous));
        client.addInProcessServer(&server);

        ToolDiscoveryBridge bridge(&client, 0);
        const ToolExecResult res = bridge.invoke(
            QStringLiteral("terminal__does_not_exist"), QJsonObject{});
        QVERIFY(!res.ok);
    }

    void bridgeInvokesInCatalogueTool()
    {
        McpClient client;
        InProcessMcpServer server(QStringLiteral("terminal"));
        server.addTool(std::make_unique<FakeTool>(
            QStringLiteral("run_in_terminal"), AgentToolPermission::Dangerous,
            QStringLiteral("ran")));
        client.addInProcessServer(&server);

        ToolDiscoveryBridge bridge(&client, 0);
        const ToolExecResult res = bridge.invoke(
            QStringLiteral("terminal__run_in_terminal"), QJsonObject{});
        QVERIFY(res.ok);
        QCOMPARE(res.textContent, QStringLiteral("ran"));
    }
};

QTEST_MAIN(TestMcpToolServers)
#include "test_mcptoolservers.moc"