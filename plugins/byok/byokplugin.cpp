#include "byokplugin.h"
#include "byokprovider.h"

#include <QSettings>

PluginInfo ByokPlugin::info() const
{
    return {QStringLiteral("byok"),
            QStringLiteral("OpenAI Compatible"),
            QStringLiteral("1.0.0"),
            QStringLiteral("OpenAI-compatible providers (OpenAI, Z.ai, OpenRouter, Custom)"),
            QStringLiteral("Exorcist")};
}

void ByokPlugin::initialize(QObject *) {}
void ByokPlugin::shutdown() {}

QList<IAgentProvider *> ByokPlugin::createProviders(QObject *parent)
{
    // One-time, marker-guarded migration of legacy BYOK settings.
    QSettings settings;
    OpenAICompat::migrateLegacy(settings);

    QList<IAgentProvider *> providers;
    for (const OpenAICompat::Preset &preset : OpenAICompat::presets()) {
        auto *provider = new ByokProvider(preset.key);
        if (parent)
            provider->setParent(parent);
        providers.append(provider);
    }
    return providers;
}
