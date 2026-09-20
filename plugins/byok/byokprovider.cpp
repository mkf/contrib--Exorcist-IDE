#include "byokprovider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QSettings>
#include <QTimer>

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
