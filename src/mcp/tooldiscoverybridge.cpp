#include "tooldiscoverybridge.h"

#include "mcptooladapter.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

namespace {

// ── Bridge meta-tools ─────────────────────────────────────────────────────────

class SearchToolsTool : public ITool
{
public:
    explicit SearchToolsTool(ToolDiscoveryBridge *bridge) : m_bridge(bridge) {}

    ToolSpec spec() const override
    {
        ToolSpec s;
        s.name        = QStringLiteral("search_tools");
        s.description = QStringLiteral(
            "Search the hosted MCP tool catalogue by keyword. Returns compact "
            "matches (name, server, description). Use get_tool_details to "
            "retrieve a tool's full schema before invoking it.");
        s.permission  = AgentToolPermission::ReadOnly;
        s.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("query"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("description"), QStringLiteral("Search keywords.")}
                }},
                {QStringLiteral("limit"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("integer")},
                    {QStringLiteral("description"), QStringLiteral("Maximum results. Default: 10.")}
                }}
            }},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("query")}}
        };
        return s;
    }

    ToolExecResult invoke(const QJsonObject &args) override
    {
        const QString query = args.value(QLatin1String("query")).toString();
        const int limit = args.value(QLatin1String("limit")).toInt(10);
        const QJsonArray results = m_bridge->search(query, limit);
        ToolExecResult r;
        r.ok = true;
        r.data = QJsonObject{{QStringLiteral("results"), results}};
        r.textContent = QStringLiteral("%1 tool(s) matched.").arg(results.size());
        return r;
    }

private:
    ToolDiscoveryBridge *m_bridge;
};

class GetToolDetailsTool : public ITool
{
public:
    explicit GetToolDetailsTool(ToolDiscoveryBridge *bridge) : m_bridge(bridge) {}

    ToolSpec spec() const override
    {
        ToolSpec s;
        s.name        = QStringLiteral("get_tool_details");
        s.description = QStringLiteral(
            "Return the full definition (including input schema) of a hosted "
            "tool by its namespaced name.");
        s.permission  = AgentToolPermission::ReadOnly;
        s.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("name"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("description"), QStringLiteral("Namespaced tool name, e.g. terminal__run_in_terminal.")}
                }}
            }},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("name")}}
        };
        return s;
    }

    ToolExecResult invoke(const QJsonObject &args) override
    {
        const QString name = args.value(QLatin1String("name")).toString();
        const QJsonObject details = m_bridge->details(name);
        if (details.isEmpty())
            return {false, {}, {}, QStringLiteral("Tool '%1' is not in the current catalogue").arg(name)};
        ToolExecResult r;
        r.ok = true;
        r.data = details;
        r.textContent = QStringLiteral("Definition for %1.").arg(name);
        return r;
    }

private:
    ToolDiscoveryBridge *m_bridge;
};

class InvokeToolTool : public ITool
{
public:
    explicit InvokeToolTool(ToolDiscoveryBridge *bridge) : m_bridge(bridge) {}

    ToolSpec spec() const override
    {
        ToolSpec s;
        s.name        = QStringLiteral("invoke_tool");
        s.description = QStringLiteral(
            "Invoke a hosted tool by its namespaced name. The tool must be in "
            "the current catalogue; arguments are validated by the host.");
        s.permission  = AgentToolPermission::Dangerous;
        s.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{
                {QStringLiteral("name"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("string")},
                    {QStringLiteral("description"), QStringLiteral("Namespaced tool name.")}
                }},
                {QStringLiteral("arguments"), QJsonObject{
                    {QStringLiteral("type"), QStringLiteral("object")},
                    {QStringLiteral("description"), QStringLiteral("Tool arguments.")}
                }}
            }},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("name")}}
        };
        return s;
    }

    ToolExecResult invoke(const QJsonObject &args) override
    {
        const QString name = args.value(QLatin1String("name")).toString();
        const QJsonObject toolArgs = args.value(QLatin1String("arguments")).toObject();
        return m_bridge->invoke(name, toolArgs);
    }

private:
    ToolDiscoveryBridge *m_bridge;
};

} // namespace

// ── ToolDiscoveryBridge ───────────────────────────────────────────────────────

ToolDiscoveryBridge::ToolDiscoveryBridge(McpClient *client, int threshold)
    : m_client(client), m_threshold(threshold)
{
}

ToolDiscoveryBridge::~ToolDiscoveryBridge() = default;

int ToolDiscoveryBridge::catalogueSize() const
{
    return m_client ? m_client->allTools().size() : 0;
}

bool ToolDiscoveryBridge::isActive() const
{
    return catalogueSize() > m_threshold;
}

std::vector<std::unique_ptr<ITool>> ToolDiscoveryBridge::createBridgeTools()
{
    std::vector<std::unique_ptr<ITool>> tools;
    if (!isActive())
        return tools;

    tools.push_back(std::make_unique<SearchToolsTool>(this));
    tools.push_back(std::make_unique<GetToolDetailsTool>(this));
    tools.push_back(std::make_unique<InvokeToolTool>(this));
    return tools;
}

void ToolDiscoveryBridge::ensureIndex() const
{
    if (m_indexBuilt)
        return;
    // Below the threshold the host exposes hosted tools directly and no
    // search index is built.
    if (!isActive())
        return;

    m_indexBuilt = true;
    m_index.clear();

    const QList<McpToolInfo> tools = m_client->allTools();
    m_index.reserve(static_cast<size_t>(tools.size()));
    for (const McpToolInfo &t : tools) {
        Entry e;
        e.namespacedName = McpToolAdapter::namespacedName(t.serverName, t.name);
        e.serverName     = t.serverName;
        e.localName      = t.name;
        e.description    = t.description;
        e.inputSchema    = t.inputSchema;
        m_index.push_back(e);
    }
}

const ToolDiscoveryBridge::Entry *
ToolDiscoveryBridge::findEntry(const QString &namespacedName) const
{
    for (const Entry &e : m_index) {
        if (e.namespacedName == namespacedName)
            return &e;
    }
    return nullptr;
}

QJsonArray ToolDiscoveryBridge::search(const QString &query, int limit) const
{
    ensureIndex();

    QJsonArray results;
    const QString q = query.trimmed().toLower();
    for (const Entry &e : m_index) {
        if (results.size() >= limit)
            break;
        if (q.isEmpty()
            || e.namespacedName.toLower().contains(q)
            || e.description.toLower().contains(q)) {
            QJsonObject r;
            r[QLatin1String("name")]        = e.namespacedName;
            r[QLatin1String("server")]      = e.serverName;
            r[QLatin1String("description")] = e.description;
            results.append(r);
        }
    }
    return results;
}

QJsonObject ToolDiscoveryBridge::details(const QString &namespacedName) const
{
    ensureIndex();

    const Entry *e = findEntry(namespacedName);
    if (!e)
        return {};

    QJsonObject d;
    d[QLatin1String("name")]        = e->namespacedName;
    d[QLatin1String("server")]      = e->serverName;
    d[QLatin1String("tool")]        = e->localName;
    d[QLatin1String("description")] = e->description;
    d[QLatin1String("inputSchema")] = e->inputSchema;
    return d;
}

ToolExecResult ToolDiscoveryBridge::invoke(const QString &namespacedName,
                                           const QJsonObject &args)
{
    ensureIndex();

    const Entry *e = findEntry(namespacedName);
    if (!e) {
        return {false, {}, {},
                QStringLiteral("Tool '%1' is not in the current catalogue")
                    .arg(namespacedName)};
    }

    QEventLoop loop;
    McpToolResult mcpResult;
    bool finished = false;
    bool timedOut = false;
    const QString reqId = QStringLiteral("bridge-call-%1").arg(quintptr(this));

    auto conn = QObject::connect(
        m_client, &McpClient::toolCallFinished,
        [&](const QString &id, const McpToolResult &res) {
            if (timedOut || id != reqId)
                return;
            mcpResult = res;
            finished = true;
            loop.quit();
        });

    m_client->callTool(e->localName, args, reqId);
    if (!finished) {
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, [&]() {
            timedOut = true;
            mcpResult.ok = false;
            mcpResult.error = QStringLiteral("Tool call timed out.");
            loop.quit();
        });
        timer.start(60000);
        loop.exec();
        timer.stop();
    }
    QObject::disconnect(conn);

    ToolExecResult result;
    result.ok = mcpResult.ok;
    result.textContent = mcpResult.text;
    result.error = mcpResult.error;
    if (!mcpResult.content.isEmpty()) {
        QJsonObject data;
        data[QLatin1String("content")] = mcpResult.content;
        result.data = data;
    }
    return result;
}