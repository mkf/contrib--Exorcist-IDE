#pragma once

#include <QObject>
#include <QString>

#include "plugininterface.h"

class IHostServices;
namespace jssdk { class JsPluginRuntime; }

class JsPluginSdkPlugin : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID EXORCIST_PLUGIN_IID)
    Q_INTERFACES(IPlugin)

public:
    PluginInfo info() const override;
    bool initialize(IHostServices *host) override;
    void shutdown() override;

private:
    IHostServices *m_host = nullptr;
    std::unique_ptr<jssdk::JsPluginRuntime> m_runtime;
};
