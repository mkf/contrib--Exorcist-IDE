#pragma once

#include <QWidget>
#include "iagentuirenderer.h"
#include "agentuievent.h"

class DashboardJSBridge;
class QVBoxLayout;
class QLabel;
class QListWidget;

// ── AgentDashboardPanel — live operational dashboard for agent missions ──────
//
// Implements IAgentUIRenderer. Renders steps, metrics, logs, and artifacts
// using basic Qt widgets.
//
// This panel sits alongside the chat panel — chat = conversation,
// dashboard = operational status.

class AgentDashboardPanel : public QWidget, public IAgentUIRenderer
{
    Q_OBJECT

public:
    explicit AgentDashboardPanel(QWidget *parent = nullptr);

    // ── IAgentUIRenderer ──────────────────────────────────────────────────
    void handleEvent(const AgentUIEvent &event) override;
    void clearDashboard() override;
    bool isActive() const override;

signals:
    void openFileRequested(const QString &path);

private:
    void buildUi();
    QString eventTypeName(AgentUIEventType type) const;

    QLabel      *m_titleLabel = nullptr;
    QLabel      *m_statusLabel = nullptr;
    QListWidget *m_stepsList = nullptr;
    QListWidget *m_logsList = nullptr;

    DashboardJSBridge *m_bridge = nullptr;
    QVBoxLayout       *m_layout = nullptr;
};
