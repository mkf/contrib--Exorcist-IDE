#include "inprocessmcpserver.h"

#include "../agent/itool.h"

InProcessMcpServer::InProcessMcpServer(const QString &name)
    : m_name(name)
{
}

InProcessMcpServer::~InProcessMcpServer() = default;

QString InProcessMcpServer::normalizeToolName(const QString &rawName)
{
    QString name = rawName;

    // Strip repeated transport prefixes (mcp_, mcp_mcp_, ...).
    while (name.startsWith(QLatin1String("mcp_")))
        name.remove(0, 4);

    // Strip known vendor/server prefixes. Applied repeatedly so combined
    // prefixes (for example docker_browser_) collapse to the operation.
    static const QStringList vendorPrefixes = {
        QStringLiteral("copilot_conta_"),
        QStringLiteral("docker_browser_"),
        QStringLiteral("docker_mcp_"),
        QStringLiteral("pylance_mcp_s_"),
        QStringLiteral("copilot_"),
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (const QString &prefix : vendorPrefixes) {
            if (name.startsWith(prefix)) {
                name.remove(0, prefix.size());
                changed = true;
                break;
            }
        }
    }

    return name;
}

bool InProcessMcpServer::addTool(std::unique_ptr<ITool> tool)
{
    if (!tool)
        return false;

    const QString name = normalizeToolName(tool->spec().name);
    if (name.isEmpty() || m_byName.contains(name))
        return false;

    ITool *raw = tool.get();
    m_tools.push_back(std::move(tool));
    m_byName.insert(name, raw);
    return true;
}

QJsonArray InProcessMcpServer::listTools() const
{
    QJsonArray arr;
    for (const auto &tool : m_tools) {
        const ToolSpec spec = tool->spec();
        QJsonObject def;
        def[QLatin1String("name")]        = normalizeToolName(spec.name);
        def[QLatin1String("description")] = spec.description;
        def[QLatin1String("inputSchema")] = spec.inputSchema;
        if (!spec.outputSchema.isEmpty())
            def[QLatin1String("outputSchema")] = spec.outputSchema;
        // Non-standard extensions: let the host preserve the tool's
        // permission level and context scoping when adapting it into the
        // agent tool registry.
        def[QLatin1String("x-permission")] = static_cast<int>(spec.permission);
        if (!spec.contexts.isEmpty())
            def[QLatin1String("x-contexts")] = QJsonArray::fromStringList(spec.contexts);
        arr.append(def);
    }
    return arr;
}

QJsonObject InProcessMcpServer::callTool(const QString &name,
                                         const QJsonObject &arguments)
{
    ITool *handler = m_byName.value(name, nullptr);
    if (!handler) {
        QJsonObject text;
        text[QLatin1String("type")] = QStringLiteral("text");
        text[QLatin1String("text")] =
            QStringLiteral("Unknown tool '%1' on server '%2'").arg(name, m_name);

        QJsonArray content;
        content.append(text);

        QJsonObject out;
        out[QLatin1String("content")] = content;
        out[QLatin1String("isError")] = true;
        return out;
    }

    const ToolExecResult res = handler->invoke(arguments);

    QJsonObject text;
    text[QLatin1String("type")] = QStringLiteral("text");
    text[QLatin1String("text")] = res.ok ? res.textContent : res.error;

    QJsonArray content;
    content.append(text);

    QJsonObject out;
    out[QLatin1String("content")] = content;
    if (!res.ok)
        out[QLatin1String("isError")] = true;
    if (!res.data.isEmpty())
        out[QLatin1String("structuredContent")] = res.data;
    return out;
}

QStringList InProcessMcpServer::toolNames() const
{
    QStringList names;
    names.reserve(static_cast<int>(m_tools.size()));
    for (const auto &tool : m_tools)
        names.append(normalizeToolName(tool->spec().name));
    return names;
}

bool InProcessMcpServer::hasTool(const QString &name) const
{
    return m_byName.contains(name);
}

ITool *InProcessMcpServer::tool(const QString &name) const
{
    return m_byName.value(name, nullptr);
}