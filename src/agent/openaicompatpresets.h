#pragma once

// ── OpenAI-compatible provider presets ───────────────────────────────────────
//
// Shared, header-only registry and helpers used by both the host settings UI
// and the `byok` (OpenAI-compatible) plugin. Keeping everything inline avoids
// a link dependency between the plugin DSO and the host binary while still
// giving both the same defaults, persistence keys and URL derivation.
//
// Preset identity:
//   key  → short settings segment, e.g. "openai"
//   id   → registered provider id,  e.g. "openai-compat:openai"
//
// Settings layout (QSettings):
//   AI/OpenAICompatible/activePreset         (key)
//   AI/OpenAICompatible/migrated             (bool marker)
//   AI/OpenAICompatible/<key>/endpoint       (overrides the preset default when non-empty)
//   AI/OpenAICompatible/<key>/apiKey
//   AI/OpenAICompatible/<key>/model
//   AI/OpenAICompatible/<key>/models         (JSON array of discovered model ids)

#include <QList>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QUrl>

namespace OpenAICompat {

struct Preset
{
    QString key;             // settings segment, e.g. "openai"
    QString id;              // registered provider id, e.g. "openai-compat:openai"
    QString displayName;     // dropdown label
    QString defaultEndpoint; // default chat-completions URL (empty for Custom)
};

/// The fixed, ordered preset registry.
inline const QList<Preset> &presets()
{
    static const QList<Preset> list = {
        {QStringLiteral("openai"),
         QStringLiteral("openai-compat:openai"),
         QStringLiteral("OpenAI"),
         QStringLiteral("https://api.openai.com/v1/chat/completions")},
        {QStringLiteral("zai"),
         QStringLiteral("openai-compat:zai"),
         QStringLiteral("Z.ai"),
         QStringLiteral("https://api.z.ai/api/coding/paas/v4/chat/completions")},
        {QStringLiteral("openrouter"),
         QStringLiteral("openai-compat:openrouter"),
         QStringLiteral("OpenRouter"),
         QStringLiteral("https://openrouter.ai/api/v1/chat/completions")},
        {QStringLiteral("custom"),
         QStringLiteral("openai-compat:custom"),
         QStringLiteral("Custom"),
         QString()},
    };
    return list;
}

inline const Preset *presetByKey(const QString &key)
{
    for (const Preset &p : presets())
        if (p.key == key)
            return &p;
    return nullptr;
}

inline const Preset *presetById(const QString &id)
{
    for (const Preset &p : presets())
        if (p.id == id)
            return &p;
    return nullptr;
}

/// Default preset when nothing has been saved yet (spec: Custom).
inline const Preset *defaultPreset() { return presetByKey(QStringLiteral("custom")); }

/// Effective endpoint: the saved value when non-empty, otherwise the default.
inline QString effectiveEndpoint(const Preset &p, const QString &savedEndpoint)
{
    const QString saved = savedEndpoint.trimmed();
    return saved.isEmpty() ? p.defaultEndpoint : saved;
}

/// Map a completions URL host to a preset key: openai.com → openai,
/// openrouter.ai → openrouter, otherwise custom.
inline QString classifyHost(const QString &url)
{
    const QString host = QUrl(url.trimmed()).host().toLower();
    if (host == QLatin1String("openrouter.ai") || host.endsWith(QLatin1String(".openrouter.ai")))
        return QStringLiteral("openrouter");
    if (host == QLatin1String("openai.com") || host.endsWith(QLatin1String(".openai.com")))
        return QStringLiteral("openai");
    return QStringLiteral("custom");
}

/// Derive a model-list URL from a chat-completions URL by dropping the query
/// and the trailing operation segment, then appending "models". Returns an
/// empty string when the input is empty. Best-effort: callers must tolerate a
/// request to the derived URL failing.
inline QString deriveModelsUrl(const QString &completionsUrl)
{
    QString base = completionsUrl.trimmed();
    if (base.isEmpty())
        return QString();

    const int query = base.indexOf(QLatin1Char('?'));
    if (query >= 0)
        base = base.left(query);

    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);

    static const QStringList ops = {
        QStringLiteral("/chat/completions"),
        QStringLiteral("/completions"),
        QStringLiteral("/messages"),
        QStringLiteral("/responses"),
    };
    for (const QString &op : ops) {
        if (base.endsWith(op)) {
            base.chop(op.size());
            break;
        }
    }

    if (base.isEmpty())
        return QString();
    return base + QStringLiteral("/models");
}

// ── Settings keys ─────────────────────────────────────────────────────────────

inline QString settingsRoot() { return QStringLiteral("AI/OpenAICompatible"); }
inline QString endpointKey(const QString &k) { return settingsRoot() + QLatin1Char('/') + k + QStringLiteral("/endpoint"); }
inline QString apiKeyKey(const QString &k)   { return settingsRoot() + QLatin1Char('/') + k + QStringLiteral("/apiKey"); }
inline QString modelKey(const QString &k)    { return settingsRoot() + QLatin1Char('/') + k + QStringLiteral("/model"); }
inline QString modelsKey(const QString &k)   { return settingsRoot() + QLatin1Char('/') + k + QStringLiteral("/models"); }
inline QString activePresetKey()             { return settingsRoot() + QStringLiteral("/activePreset"); }
inline QString migratedKey()                 { return settingsRoot() + QStringLiteral("/migrated"); }

// Legacy BYOK keys (pre-preset). Read-only migration inputs.
inline QString legacyEndpointKey() { return QStringLiteral("AI/customEndpoint"); }
inline QString legacyApiKeyKey()   { return QStringLiteral("AI/customApiKey"); }

// ── Settings accessors ────────────────────────────────────────────────────────

inline QString loadEndpoint(QSettings &s, const QString &k) { return s.value(endpointKey(k)).toString(); }
inline QString loadApiKey(QSettings &s, const QString &k)   { return s.value(apiKeyKey(k)).toString(); }
inline QString loadModel(QSettings &s, const QString &k)    { return s.value(modelKey(k)).toString(); }
inline QStringList loadModels(QSettings &s, const QString &k)
{
    return s.value(modelsKey(k)).toStringList();
}

inline void saveEndpoint(QSettings &s, const QString &k, const QString &v) { s.setValue(endpointKey(k), v.trimmed()); }
inline void saveApiKey(QSettings &s, const QString &k, const QString &v)   { s.setValue(apiKeyKey(k), v); }
inline void saveModel(QSettings &s, const QString &k, const QString &v)    { s.setValue(modelKey(k), v); }
inline void saveModels(QSettings &s, const QString &k, const QStringList &v) { s.setValue(modelsKey(k), v); }

inline QString loadActivePreset(QSettings &s)
{
    return s.value(activePresetKey(), QStringLiteral("custom")).toString();
}
inline void saveActivePreset(QSettings &s, const QString &k) { s.setValue(activePresetKey(), k); }

// ── Migration ─────────────────────────────────────────────────────────────────

/// One-time, marker-guarded migration of the legacy BYOK settings into the
/// preset namespace (spec: Migration of legacy BYOK configuration).
/// Non-destructive: the legacy keys are left untouched. Idempotent: once the
/// marker is set, subsequent runs are no-ops and never overwrite user edits.
inline void migrateLegacy(QSettings &s)
{
    if (s.value(migratedKey(), false).toBool())
        return;

    const QString legacyEndpoint = s.value(legacyEndpointKey()).toString().trimmed();
    const QString legacyApiKey   = s.value(legacyApiKeyKey()).toString();

    if (legacyEndpoint.isEmpty() && legacyApiKey.isEmpty()) {
        s.setValue(migratedKey(), true);
        return;
    }

    const QString target = legacyEndpoint.isEmpty()
        ? QStringLiteral("custom")
        : classifyHost(legacyEndpoint);

    if (!legacyEndpoint.isEmpty())
        saveEndpoint(s, target, legacyEndpoint);
    if (!legacyApiKey.isEmpty())
        saveApiKey(s, target, legacyApiKey);
    saveActivePreset(s, target);
    s.setValue(migratedKey(), true);
}

} // namespace OpenAICompat