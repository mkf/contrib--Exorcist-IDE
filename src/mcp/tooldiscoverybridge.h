#pragma once

#include "../agent/itool.h"
#include "mcpclient.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <memory>
#include <vector>

// ── ToolDiscoveryBridge ───────────────────────────────────────────────────────
//
// Host-side progressive discovery for a large aggregate MCP catalogue.
//
// When the catalogue exceeds a configurable threshold, the host exposes three
// compact meta-tools (search_tools, get_tool_details, invoke_tool) instead of
// injecting every hosted tool schema into the model context. Below the
// threshold the bridge is inactive and the host exposes the hosted tools
// directly.
//
// The search index is built lazily on first discovery request and only when
// the bridge is active.

class ToolDiscoveryBridge
{
public:
    explicit ToolDiscoveryBridge(McpClient *client, int threshold = 40);
    ~ToolDiscoveryBridge();

    ToolDiscoveryBridge(const ToolDiscoveryBridge &) = delete;
    ToolDiscoveryBridge &operator=(const ToolDiscoveryBridge &) = delete;

    void setThreshold(int threshold) { m_threshold = threshold; }
    int threshold() const { return m_threshold; }

    /// True when the aggregate catalogue exceeds the threshold.
    bool isActive() const;

    /// Create the three bridge meta-tools. The caller takes ownership.
    /// Returns an empty vector when the bridge is inactive.
    std::vector<std::unique_ptr<ITool>> createBridgeTools();

    /// Search the catalogue. Returns compact results (name, server, description).
    QJsonArray search(const QString &query, int limit = 10) const;

    /// Full definition (including inputSchema) for a namespaced tool name.
    QJsonObject details(const QString &namespacedName) const;

    /// Invoke a namespaced tool. Rejects names outside the current catalogue.
    ToolExecResult invoke(const QString &namespacedName, const QJsonObject &args);

    /// Number of tools in the aggregate catalogue.
    int catalogueSize() const;

    /// Whether the lazy search index has been built.
    bool indexBuilt() const { return m_indexBuilt; }

private:
    struct Entry {
        QString     namespacedName;
        QString     serverName;
        QString     localName;
        QString     description;
        QJsonObject inputSchema;
    };

    void ensureIndex() const;
    const Entry *findEntry(const QString &namespacedName) const;

    McpClient *m_client;
    int        m_threshold;

    mutable bool               m_indexBuilt = false;
    mutable std::vector<Entry> m_index;
};