#include "dashboardjsbridge.h"

#include <QJsonDocument>

DashboardJSBridge::DashboardJSBridge(QObject *parent)
    : QObject(parent)
{
}

void DashboardJSBridge::pushEvent(const QString &typeName,
                                   const QString &missionId,
                                   qint64 timestamp,
                                   const QJsonObject &payload)
{
    QJsonObject envelope;
    envelope[QStringLiteral("type")]      = typeName;
    envelope[QStringLiteral("missionId")] = missionId;
    envelope[QStringLiteral("timestamp")] = timestamp;
    envelope[QStringLiteral("payload")]   = payload;

    const QByteArray json = QJsonDocument(envelope).toJson(QJsonDocument::Compact);
    const QString escaped = QString::fromUtf8(json)
        .replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
        .replace(QLatin1Char('\''), QStringLiteral("\\'"));

    eval(QStringLiteral("dashboardBridge.handleEvent('%1');").arg(escaped));
}

void DashboardJSBridge::clearDashboard()
{
    eval(QStringLiteral("dashboardBridge.clear();"));
}

void DashboardJSBridge::setTheme(const QJsonObject &themeTokens)
{
    const QByteArray json = QJsonDocument(themeTokens).toJson(QJsonDocument::Compact);
    const QString escaped = QString::fromUtf8(json)
        .replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
        .replace(QLatin1Char('\''), QStringLiteral("\\'"));
    eval(QStringLiteral("dashboardBridge.setTheme('%1');").arg(escaped));
}

void DashboardJSBridge::eval(const QString &js)
{
    Q_UNUSED(js);
}
