#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <memory>
#include <vector>

class ITool;

// ── InProcessMcpServer ────────────────────────────────────────────────────────
//
// A named collection of ITool handlers that answers the MCP tools/list and
// tools/call contract without spawning a child process.
//
// The server is a thin protocol shell: it owns the ITool instances it is given
// and delegates every call to ITool::invoke(). It creates no collaborators,
// threads, timers, or processes of its own, and tools/list reads ITool::spec()
// without side effects.
//
// Server-local tool names are normalized (transport/vendor prefixes stripped)
// so the host can expose them as <server>__<tool>.

class InProcessMcpServer
{
public:
    explicit InProcessMcpServer(const QString &name);
    virtual ~InProcessMcpServer();

    InProcessMcpServer(const InProcessMcpServer &) = delete;
    InProcessMcpServer &operator=(const InProcessMcpServer &) = delete;

    QString name() const { return m_name; }

    /// Takes ownership of the tool. Returns false when the normalized
    /// server-local name is empty or collides with an existing handler.
    bool addTool(std::unique_ptr<ITool> tool);

    /// MCP tools/list: typed tool definitions (name, description, inputSchema,
    /// outputSchema when present, and an x-permission extension).
    QJsonArray listTools() const;

    /// MCP tools/call: dispatch by exact server-local name. Returns an MCP
    /// tool result object (content array, optional isError, optional
    /// structuredContent).
    QJsonObject callTool(const QString &name, const QJsonObject &arguments);

    QStringList toolNames() const;
    bool hasTool(const QString &name) const;
    ITool *tool(const QString &name) const;

    /// Strips transport and vendor prefixes from an imported tool name.
    /// Example: mcp_copilot_conta_list_containers -> list_containers.
    static QString normalizeToolName(const QString &rawName);

private:
    QString m_name;
    std::vector<std::unique_ptr<ITool>> m_tools;
    QHash<QString, ITool *> m_byName;
};