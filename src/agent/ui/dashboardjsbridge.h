#pragma once

#include <QObject>
#include <QJsonObject>
#include <QVector>

// ── DashboardJSBridge — C++ ↔ JS bridge for agent dashboard ─────────────────
//
// Pushes structured AgentUIEvents to the dashboard's JavaScript as JSON
// strings. Receives user actions (artifact clicks) back from JS.
//
// In the Qt-widget dashboard there is no JS renderer, so the push methods
// are no-ops.

class DashboardJSBridge : public QObject
{
    Q_OBJECT
public:
    explicit DashboardJSBridge(QObject *parent = nullptr);

    // ── C++ → JS ─────────────────────────────────────────────────────────

    /// Push a structured event to the JS dashboard renderer.
    void pushEvent(const QString &typeName,
                   const QString &missionId,
                   qint64 timestamp,
                   const QJsonObject &payload);

    /// Clear the entire dashboard.
    void clearDashboard();

    /// Update theme CSS custom properties.
    void setTheme(const QJsonObject &themeTokens);

signals:
    // ── JS → C++ ─────────────────────────────────────────────────────────
    void openArtifactRequested(const QString &path, const QString &type);

private:
    void eval(const QString &js);
};
