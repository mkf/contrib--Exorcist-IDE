#pragma once

#include "../itool.h"

#include <QJsonObject>
#include <QString>

// A hosted tool with no backend in this build. It reports itself as
// unavailable rather than failing the hosting server.
class UnavailableTool : public ITool
{
public:
    UnavailableTool(const QString &name, const QString &description)
        : m_name(name), m_description(description) {}

    ToolSpec spec() const override
    {
        ToolSpec s;
        s.name        = m_name;
        s.description = m_description;
        s.permission  = AgentToolPermission::Dangerous;
        s.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"), QJsonObject{}},
        };
        return s;
    }

    ToolExecResult invoke(const QJsonObject &) override
    {
        return {false, {}, {},
                QStringLiteral("Tool '%1' is not available in this build.").arg(m_name)};
    }

private:
    QString m_name;
    QString m_description;
};