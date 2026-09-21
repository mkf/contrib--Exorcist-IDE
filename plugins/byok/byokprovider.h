#pragma once

#include "aiinterface.h"
#include "agent/openaicompatpresets.h"

#include <QNetworkAccessManager>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTimer>

class QNetworkReply;
class QTcpServer;

/// An OpenAI-compatible provider bound to one preset (OpenAI, Z.ai, OpenRouter
/// or Custom). Endpoint and API key are read from / written to the preset
/// namespace in QSettings via OpenAICompat helpers.
class ByokProvider : public IAgentProvider
{
    Q_OBJECT

public:
    explicit ByokProvider(QString presetKey, QObject *parent = nullptr);

    QString           id()           const override;
    QString           displayName()  const override;
    AgentCapabilities capabilities() const override;
    bool              isAvailable()  const override;

    QStringList       availableModels() const override;
    QString           currentModel()    const override;
    void              setModel(const QString &model) override;
    QList<ModelInfo>  modelInfoList() const override;

    void initialize() override;
    void shutdown()   override;

    ProviderAuthInfo authInfo() const override;
    void             startAuth() override;

    void sendRequest(const AgentRequest &request)  override;
    void cancelRequest(const QString &requestId)   override;

    /// Re-read endpoint/key/model from QSettings and refresh availability.
    Q_INVOKABLE void reloadConfig();

    /// The effective chat-completions endpoint (saved value or preset default).
    QString effectiveEndpoint() const;
    /// The API key for this preset.
    QString apiKey() const;

private:
    void connectReply(QNetworkReply *reply);
    void cleanupActiveReply();
    void fetchModels();
    QString buildUserContent(const AgentRequest &req) const;
    QString modelsUrl() const;

    // ── OpenRouter OAuth PKCE ─────────────────────────────────────────────
    void beginOpenRouterOAuth();
    void handleOpenRouterCallback();
    void exchangeOpenRouterCode(const QString &code, const QString &verifier);
    void finishOpenRouterOAuth(bool ok);

    QString m_presetKey;
    QString m_endpoint;     // saved (possibly empty)
    QString m_apiKey;       // saved
    QString m_model;        // selected model
    QStringList m_models;   // discovered (persisted) model list
    bool    m_available = false;

    QNetworkAccessManager m_nam;

    QPointer<QNetworkReply> m_activeReply;
    QString        m_activeRequestId;
    QString        m_streamAccum;

    // OpenRouter OAuth state
    QTcpServer *m_oauthServer = nullptr;
    QString     m_oauthVerifier;
    QTimer      m_oauthTimeout;
};
