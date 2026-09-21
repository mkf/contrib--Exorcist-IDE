#include "jspluginsdkplugin.h"
#include "jspluginruntime.h"

#include "sdk/ihostservices.h"
#include "sdk/iviewservice.h"

#include <QCoreApplication>
#include <QDir>

PluginInfo JsPluginSdkPlugin::info() const
{
    return {
        QStringLiteral("org.exorcist.javascript-sdk"),
        QStringLiteral("JavaScript Plugin SDK"),
        QStringLiteral("1.0.0"),
        QStringLiteral("JavaScriptCore-based runtime for lightweight JavaScript plugins"),
        QStringLiteral("Exorcist"),
        QStringLiteral("1.0"),
        {} // no special permissions — each JS plugin declares its own
    };
}

bool JsPluginSdkPlugin::initialize(IHostServices *host)
{
    m_host = host;
    m_runtime = std::make_unique<jssdk::JsPluginRuntime>(host, this);

    // Load JS plugins from <app>/plugins/javascript
    const QString jsPluginsDir =
        QCoreApplication::applicationDirPath() + QStringLiteral("/plugins/javascript");

    if (QDir(jsPluginsDir).exists()) {
        const int count = m_runtime->loadPluginsFrom(jsPluginsDir);
        if (count > 0) {
            m_runtime->initializeAll();
            m_runtime->enableHotReload(jsPluginsDir);
        }

        for (const QString &err : m_runtime->errors())
            qWarning("[JsSDK] %s", qUtf8Printable(err));
    }

    return true;
}

void JsPluginSdkPlugin::shutdown()
{
    if (m_runtime) {
        m_runtime->shutdownAll();
        m_runtime.reset();
    }
}

