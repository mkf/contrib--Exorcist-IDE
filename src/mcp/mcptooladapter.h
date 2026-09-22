#pragma once

#include "../agent/itool.h"
#include "mcpclient.h"

#include <QEventLoop>
#include <QTimer>

/// Adapter that wraps an MCP tool (remote or in-process) as an ITool so it can
/// be used by the AgentController / ToolRegistry just like built-in tools.
///
/// The model-facing name is namespaced as <server>__<tool> to avoid collisions
/// between servers. The originating server and server-local tool name are kept
/// for routing and audit.
class McpToolAdapter : public ITool
{
public:
    McpToolAdapter(McpClient *client, const McpToolInfo &info)
        : m_client(client), m_info(info) {}

    /// Model-facing namespaced name for a server-local tool.
    static QString namespacedName(const QString &serverName, const QString &toolName)
    {
        return QStringLiteral("%1__%2").arg(serverName, toolName);
    }

    /// Originating server name.
    QString serverName() const { return m_info.serverName; }

    /// Server-local tool name (the name sent on the wire).
    QString serverLocalName() const { return m_info.name; }

    ToolSpec spec() const override
    {
        ToolSpec s;
        s.name        = namespacedName(m_info.serverName, m_info.name);
        s.description = m_info.description;
        s.inputSchema = m_info.inputSchema;
        s.permission  = m_info.permission;
        s.contexts    = m_info.contexts;
        s.timeoutMs   = 60000;
        return s;
    }

    ToolExecResult invoke(const QJsonObject &args) override
    {
        // Synchronous call using an event loop. In-process servers complete
        // synchronously, so the loop is only entered when the call is still
        // pending (remote servers).
        QEventLoop loop;
        McpToolResult mcpResult;
        bool finished = false;
        bool timedOut = false;
        const QString reqId = QStringLiteral("mcp-call-%1").arg(quintptr(this));

        auto conn = QObject::connect(m_client, &McpClient::toolCallFinished,
                                     [&](const QString &id, const McpToolResult &res) {
            if (timedOut || id != reqId)
                return;
            mcpResult = res;
            finished = true;
            loop.quit();
        });

        m_client->callTool(m_info.name, args, reqId);
        if (!finished) {
            QTimer timer;
            timer.setSingleShot(true);
            QObject::connect(&timer, &QTimer::timeout, [&]() {
                timedOut = true;
                mcpResult.ok = false;
                mcpResult.error = QStringLiteral("MCP tool call timed out.");
                loop.quit();
            });
            timer.start(spec().timeoutMs);
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

private:
    McpClient   *m_client;
    McpToolInfo  m_info;
};
