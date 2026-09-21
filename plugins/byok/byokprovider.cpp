#include "byokprovider.h"

#include "agent/pkce.h"

#include <QDesktopServices>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QRandomGenerator>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUrlQuery>

Q_LOGGING_CATEGORY(lcByok, "exorcist.byok")

using OpenAICompat::Preset;

ByokProvider::ByokProvider(QString presetKey, QObject *parent)
    : m_presetKey(std::move(presetKey))
{
    if (parent)
        setParent(parent);
    QSettings s;
    m_endpoint = OpenAICompat::loadEndpoint(s, m_presetKey);
    m_apiKey   = OpenAICompat::loadApiKey(s, m_presetKey);
    m_model    = OpenAICompat::loadModel(s, m_presetKey);

    // OpenRouter OAuth: one server + one timeout connection for the lifetime
    // of the provider (avoids duplicate connections across repeated attempts).
    m_oauthServer = new QTcpServer(this);
    connect(m_oauthServer, &QTcpServer::newConnection,
            this, &ByokProvider::handleOpenRouterCallback);

    m_oauthTimeout.setSingleShot(true);
    m_oauthTimeout.setInterval(5 * 60 * 1000);
    connect(&m_oauthTimeout, &QTimer::timeout, this, [this] {
        qCInfo(lcByok) << "OpenRouter OAuth: timed out";
        finishOpenRouterOAuth(false);
    });
}

QString ByokProvider::id() const
{
    if (const Preset *p = OpenAICompat::presetByKey(m_presetKey))
        return p->id;
    return QStringLiteral("openai-compat:") + m_presetKey;
}

QString ByokProvider::displayName() const
{
    if (const Preset *p = OpenAICompat::presetByKey(m_presetKey))
        return p->displayName;
    return m_presetKey;
}

AgentCapabilities ByokProvider::capabilities() const
{
    return AgentCapability::Chat
         | AgentCapability::Streaming
         | AgentCapability::CodeEdit;
}

QString ByokProvider::effectiveEndpoint() const
{
    const Preset *p = OpenAICompat::presetByKey(m_presetKey);
    if (!p)
        return m_endpoint.trimmed();
    return OpenAICompat::effectiveEndpoint(*p, m_endpoint);
}

QString ByokProvider::apiKey() const
{
    return m_apiKey;
}

bool ByokProvider::isAvailable() const
{
    return m_available;
}

QStringList ByokProvider::availableModels() const
{
    if (!m_models.isEmpty())
        return m_models;
    if (!m_model.isEmpty())
        return {m_model};
    return {};
}

QString ByokProvider::currentModel() const
{
    return m_model;
}

QList<ModelInfo> ByokProvider::modelInfoList() const
{
    QList<ModelInfo> out;
    const QStringList ids = availableModels();
    out.reserve(ids.size());
    for (const QString &id : ids) {
        ModelInfo mi;
        mi.id   = id;
        mi.name = id;
        mi.vendor = displayName();
        mi.capabilities.type      = QStringLiteral("chat");
        mi.capabilities.streaming = true;
        out.append(mi);
    }
    return out;
}

void ByokProvider::setModel(const QString &model)
{
    if (m_model == model)
        return;
    m_model = model;
    QSettings s;
    OpenAICompat::saveModel(s, m_presetKey, model);
}

void ByokProvider::reloadConfig()
{
    QSettings s;
    m_endpoint = OpenAICompat::loadEndpoint(s, m_presetKey);
    m_apiKey   = OpenAICompat::loadApiKey(s, m_presetKey);

    const QString savedModel = OpenAICompat::loadModel(s, m_presetKey);
    if (!savedModel.isEmpty())
        m_model = savedModel;

    m_models = OpenAICompat::loadModels(s, m_presetKey);

    const bool available = !effectiveEndpoint().isEmpty() && !m_apiKey.isEmpty();
    if (m_available != available) {
        m_available = available;
        emit availabilityChanged(m_available);
    } else {
        m_available = available;
    }
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void ByokProvider::initialize()
{
    QSettings s;
    // Load persisted list first so the picker is populated instantly.
    m_models = OpenAICompat::loadModels(s, m_presetKey);

    reloadConfig();

    if (!m_model.isEmpty() && m_models.isEmpty())
        m_models = {m_model};

    emit modelsChanged();

    if (m_available)
        fetchModels();
}

void ByokProvider::shutdown()
{
    cancelRequest(m_activeRequestId);
    m_available = false;
}

ProviderAuthInfo ByokProvider::authInfo() const
{
    ProviderAuthInfo info;
    if (m_presetKey == QLatin1String("openrouter")) {
        info.kind        = AuthAction::OAuth;
        info.actionLabel = tr("Sign in with OpenRouter");
    } else if (m_presetKey == QLatin1String("openai")) {
        info.kind        = AuthAction::OpenUrl;
        info.actionLabel = tr("Get an OpenAI API Key");
        info.actionUrl   = QStringLiteral("https://platform.openai.com/api-keys");
    } else if (m_presetKey == QLatin1String("zai")) {
        info.kind        = AuthAction::OpenUrl;
        info.actionLabel = tr("Get a Z.ai API Key");
        info.actionUrl   = QStringLiteral("https://z.ai/manage-apikey/apikey-list");
    } else {
        // Custom: no public key page — send the user to settings.
        info.kind        = AuthAction::OpenSettings;
        info.actionLabel = tr("Open Settings to Paste API Key");
    }
    return info;
}

void ByokProvider::startAuth()
{
    if (m_presetKey == QLatin1String("openrouter"))
        beginOpenRouterOAuth();
}

// ── OpenRouter OAuth PKCE ─────────────────────────────────────────────────────

void ByokProvider::beginOpenRouterOAuth()
{
    if (m_oauthServer->isListening())
        return; // already in progress

    // PKCE verifier: 32 random bytes → 43-char base64url string.
    QByteArray raw(32, Qt::Uninitialized);
    for (int i = 0; i < raw.size(); ++i)
        raw[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    m_oauthVerifier = QString::fromLatin1(raw.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));

    if (!m_oauthServer->listen(QHostAddress::LocalHost, 0)) {
        qCWarning(lcByok) << "OpenRouter OAuth: cannot listen on localhost:"
                          << m_oauthServer->errorString();
        finishOpenRouterOAuth(false);
        return;
    }

    const QString callback = Pkce::openRouterCallbackUrl(m_oauthServer->serverPort());
    const QUrl authUrl = Pkce::openRouterAuthUrl(callback, Pkce::challenge(m_oauthVerifier));

    m_oauthTimeout.start();
    qCInfo(lcByok) << "OpenRouter OAuth: opening browser";
    QDesktopServices::openUrl(authUrl);
}

void ByokProvider::handleOpenRouterCallback()
{
    QTcpSocket *socket = m_oauthServer->nextPendingConnection();
    if (!socket)
        return;

    connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
        const QByteArray request = socket->readAll();
        const QList<QByteArray> parts = request.split('\n').value(0).trimmed().split(' ');

        QString code;
        if (parts.size() >= 2) {
            const QUrl url(QStringLiteral("http://localhost")
                           + QString::fromUtf8(parts.at(1)));
            code = QUrlQuery(url).queryItemValue(QStringLiteral("code"));
        }

        const QByteArray body = code.isEmpty()
            ? QByteArrayLiteral("<html><body><h3>Sign-in failed.</h3>"
                                "You can close this window.</body></html>")
            : QByteArrayLiteral("<html><body><h3>Signed in to OpenRouter.</h3>"
                                "You can close this window.</body></html>");
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n"
                      "Connection: close\r\n\r\n" + body);
        socket->disconnectFromHost();
        socket->deleteLater();

        if (code.isEmpty()) {
            qCWarning(lcByok) << "OpenRouter OAuth: callback had no code";
            finishOpenRouterOAuth(false);
            return;
        }
        exchangeOpenRouterCode(code);
    });
}

void ByokProvider::exchangeOpenRouterCode(const QString &code)
{
    QNetworkRequest req{QUrl(QStringLiteral("https://openrouter.ai/api/v1/auth/keys"))};
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);

    QJsonObject body;
    body[QStringLiteral("code")] = code;
    body[QStringLiteral("code_verifier")] = m_oauthVerifier;
    body[QStringLiteral("code_challenge_method")] = QStringLiteral("S256");

    QNetworkReply *reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcByok) << "OpenRouter OAuth: exchange failed:"
                              << reply->errorString();
            finishOpenRouterOAuth(false);
            return;
        }

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString key = obj[QStringLiteral("key")].toString();
        if (key.isEmpty()) {
            qCWarning(lcByok) << "OpenRouter OAuth: response had no key";
            finishOpenRouterOAuth(false);
            return;
        }

        m_apiKey = key;
        QSettings s;
        OpenAICompat::saveApiKey(s, m_presetKey, key);
        qCInfo(lcByok) << "OpenRouter OAuth: key stored";
        finishOpenRouterOAuth(true);
    });
}

void ByokProvider::finishOpenRouterOAuth(bool ok)
{
    m_oauthTimeout.stop();
    if (m_oauthServer->isListening())
        m_oauthServer->close();
    m_oauthVerifier.clear();

    if (ok) {
        reloadConfig();
        if (m_available)
            fetchModels();
    }
}

QString ByokProvider::modelsUrl() const
{
    return OpenAICompat::deriveModelsUrl(effectiveEndpoint());
}

void ByokProvider::fetchModels()
{
    const QString url = modelsUrl();
    if (url.isEmpty())
        return; // best-effort: nothing to request

    QNetworkRequest req{QUrl{url}};
    req.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    req.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return; // keep persisted list on failure (spec: non-fatal)

        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QJsonArray data = obj[QLatin1String("data")].toArray();

        QStringList models;
        for (const QJsonValue &v : data) {
            const QString id = v.toObject()[QLatin1String("id")].toString();
            if (!id.isEmpty())
                models.append(id);
        }

        if (models.isEmpty())
            return; // unsuccessful refresh — keep previous list

        models.sort();
        m_models = models;
        QSettings s;
        OpenAICompat::saveModels(s, m_presetKey, m_models);

        if (m_model.isEmpty() || !m_models.contains(m_model))
            m_model = m_models.first();

        emit modelsChanged();
    });
}

// ── Requests ──────────────────────────────────────────────────────────────────

void ByokProvider::sendRequest(const AgentRequest &request)
{
    if (!m_available) {
        AgentError err;
        err.requestId = request.requestId;
        err.code      = AgentError::Code::AuthError;
        err.message   = tr("%1 not configured. Set the endpoint URL and API key in Settings.")
                            .arg(displayName());
        emit responseError(request.requestId, err);
        return;
    }

    QJsonArray messages;
    messages.append(QJsonObject{
        {QStringLiteral("role"),    QStringLiteral("system")},
        {QStringLiteral("content"), QStringLiteral("You are an AI coding assistant.")}
    });

    for (const auto &msg : request.conversationHistory) {
        QString role;
        switch (msg.role) {
        case AgentMessage::Role::System:    role = QStringLiteral("system");    break;
        case AgentMessage::Role::User:      role = QStringLiteral("user");      break;
        case AgentMessage::Role::Assistant: role = QStringLiteral("assistant"); break;
        case AgentMessage::Role::Tool:      continue;
        }
        messages.append(QJsonObject{
            {QStringLiteral("role"),    role},
            {QStringLiteral("content"), msg.content}
        });
    }

    if (request.appendUserMessage) {
        messages.append(QJsonObject{
            {QStringLiteral("role"),    QStringLiteral("user")},
            {QStringLiteral("content"), buildUserContent(request)}
        });
    }

    const QJsonObject body{
        {QStringLiteral("model"),    m_model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("stream"),   true}
    };

    QNetworkRequest netReq{QUrl{effectiveEndpoint()}};
    netReq.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    netReq.setHeader(QNetworkRequest::ContentTypeHeader,
                     QStringLiteral("application/json"));
    netReq.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    m_activeRequestId = request.requestId;
    m_streamAccum.clear();

    QNetworkReply *reply = m_nam.post(netReq,
        QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_activeReply = reply;
    connectReply(reply);
}

void ByokProvider::cancelRequest(const QString &requestId)
{
    if (requestId.isEmpty() || requestId != m_activeRequestId)
        return;
    cleanupActiveReply();
    m_activeRequestId.clear();
}

void ByokProvider::cleanupActiveReply()
{
    if (m_activeReply) {
        m_activeReply->disconnect(this);
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;
    }
}

void ByokProvider::connectReply(QNetworkReply *reply)
{
    QPointer<QNetworkReply> safeReply(reply);

    // Stream SSE data lines
    connect(reply, &QNetworkReply::readyRead, this, [this, safeReply] {
        if (!safeReply || safeReply != m_activeReply)
            return;
        const QByteArray raw = safeReply->readAll();
        const QStringList lines = QString::fromUtf8(raw).split(QLatin1Char('\n'));

        for (const QString &line : lines) {
            if (!line.startsWith(QLatin1String("data: ")))
                continue;
            const QString data = line.mid(6).trimmed();
            if (data == QLatin1String("[DONE]")) {
                AgentResponse resp;
                resp.requestId = m_activeRequestId;
                resp.text      = m_streamAccum;
                const QString reqId = m_activeRequestId;
                m_activeRequestId.clear();
                cleanupActiveReply();
                m_streamAccum.clear();
                emit responseFinished(reqId, resp);
                return;
            }

            const QJsonObject obj = QJsonDocument::fromJson(data.toUtf8()).object();
            const QJsonArray choices = obj[QLatin1String("choices")].toArray();
            if (choices.isEmpty()) continue;

            const QString content =
                choices[0].toObject()[QLatin1String("delta")]
                    .toObject()[QLatin1String("content")].toString();
            if (!content.isEmpty()) {
                m_streamAccum += content;
                emit responseDelta(m_activeRequestId, content);
            }
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, safeReply] {
        if (!safeReply)
            return;
        if (m_activeReply != safeReply) {
            // Orphan reply — already cleaned up.
            return;
        }

        const int status =
            safeReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto error = safeReply->error();
        const QString errorStr = safeReply->errorString();

        cleanupActiveReply();

        if (m_activeRequestId.isEmpty())
            return;

        if (error == QNetworkReply::NoError) {
            AgentResponse resp;
            resp.requestId = m_activeRequestId;
            resp.text = m_streamAccum;
            const QString reqId = m_activeRequestId;
            m_activeRequestId.clear();
            m_streamAccum.clear();
            emit responseFinished(reqId, resp);
        } else {
            AgentError err;
            err.requestId = m_activeRequestId;
            err.message   = errorStr;
            if (status == 401 || status == 403)
                err.code = AgentError::Code::AuthError;
            else if (status == 429)
                err.code = AgentError::Code::RateLimited;
            else
                err.code = AgentError::Code::NetworkError;
            const QString reqId = m_activeRequestId;
            m_activeRequestId.clear();
            m_streamAccum.clear();
            emit responseError(reqId, err);
        }
    });
}

QString ByokProvider::buildUserContent(const AgentRequest &req) const
{
    QString content;
    if (!req.activeFilePath.isEmpty()) {
        content += QStringLiteral("Current file: %1\n").arg(req.activeFilePath);
        if (!req.languageId.isEmpty())
            content += QStringLiteral("Language: %1\n").arg(req.languageId);
    }
    if (!req.selectedText.isEmpty())
        content += QStringLiteral("\nSelected code:\n```\n%1\n```\n\n").arg(req.selectedText);
    content += req.userPrompt;
    return content;
}
