#include <QTest>
#include <QCoreApplication>
#include <QJsonObject>
#include <QProcess>

#include "agent/mcpservers/agenttoolservers.h"
#include "agent/tools/sandboxtools.h"
#include "mcp/inprocessmcpserver.h"

// Verifies the OOP server grouping: each server owns one cohesive tool family,
// server-local names are normalized, and stub servers degrade gracefully.

class TestMcpToolServerGrouping : public QObject
{
    Q_OBJECT

private slots:

    void hostedToolMatchesDirectImplementation()
    {
        // A hosted call must delegate to the same implementation as a direct
        // call, so behavior is preserved.
        SystemToolServer server;
        ITool *hosted = server.tool(QStringLiteral("current_time"));
        QVERIFY(hosted != nullptr);

        CurrentTimeTool direct;
        const QJsonObject args;
        const ToolExecResult directResult = direct.invoke(args);
        const QJsonObject hostedResult =
            server.callTool(QStringLiteral("current_time"), args);

        QVERIFY(directResult.ok);
        QVERIFY(!hostedResult[QStringLiteral("isError")].toBool());
        QVERIFY(!hostedResult[QStringLiteral("content")].toArray().isEmpty());
    }

    void serverConstructionCreatesNoProcesses()
    {
        // Hosting must not spawn child processes or otherwise initialize
        // resources the platform did not already create.
        const int before =
            QCoreApplication::instance()->findChildren<QProcess *>().size();

        GithubToolServer github;
        WebToolServer web;
        BrowserToolServer browser;
        PylanceToolServer pylance;
        SonarQubeToolServer sonarqube;
        DatabaseToolServer database;
        SystemToolServer system;

        const int after =
            QCoreApplication::instance()->findChildren<QProcess *>().size();
        QCOMPARE(after, before);
    }

    void githubServerExposesGithubTools()
    {
        GithubToolServer server;
        QCOMPARE(server.name(), QStringLiteral("github"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("github_issues")));
        QVERIFY(names.contains(QStringLiteral("github_pr")));
        QVERIFY(names.contains(QStringLiteral("github_code_search")));
        QVERIFY(names.contains(QStringLiteral("github_repo")));
    }

    void webServerExposesWebTools()
    {
        WebToolServer server;
        QCOMPARE(server.name(), QStringLiteral("web"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("fetch_webpage")));
        QVERIFY(names.contains(QStringLiteral("web_search")));
        QVERIFY(names.contains(QStringLiteral("http_request")));
    }

    void browserServerNormalizesNames()
    {
        BrowserToolServer server;
        QCOMPARE(server.name(), QStringLiteral("browser"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("navigate")));
        QVERIFY(names.contains(QStringLiteral("take_screenshot")));
        QVERIFY(names.contains(QStringLiteral("run_code")));
        // No transport/vendor prefixes leak into server-local names.
        for (const QString &n : names)
            QVERIFY(!n.startsWith(QLatin1String("mcp_")));
    }

    void browserServerReportsUnavailable()
    {
        BrowserToolServer server;
        const QJsonObject result = server.callTool(QStringLiteral("navigate"), QJsonObject{});
        QVERIFY(result[QStringLiteral("isError")].toBool());
        // The server stays usable for its other tools.
        QVERIFY(server.hasTool(QStringLiteral("click")));
    }

    void pylanceServerExposesNormalizedNames()
    {
        PylanceToolServer server;
        QCOMPARE(server.name(), QStringLiteral("pylance"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("pylance_documents")));
        QVERIFY(names.contains(QStringLiteral("pylance_run_code_snippet")));
        for (const QString &n : names)
            QVERIFY(!n.startsWith(QLatin1String("mcp_")));
    }

    void sonarqubeServerExposesNormalizedNames()
    {
        SonarQubeToolServer server;
        QCOMPARE(server.name(), QStringLiteral("sonarqube"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("analyze_file")));
        QVERIFY(names.contains(QStringLiteral("list_potential_security_issues")));
        QVERIFY(names.contains(QStringLiteral("exclude_from_analysis")));
        QVERIFY(names.contains(QStringLiteral("setup_connected_mode")));
    }

    void databaseServerExposesDatabaseTools()
    {
        DatabaseToolServer server;
        QCOMPARE(server.name(), QStringLiteral("database"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("database_query")));
        QVERIFY(names.contains(QStringLiteral("project_database")));
    }

    void systemServerExposesSystemTools()
    {
        SystemToolServer server;
        QCOMPARE(server.name(), QStringLiteral("system"));
        const QStringList names = server.toolNames();
        QVERIFY(names.contains(QStringLiteral("process_manager")));
        QVERIFY(names.contains(QStringLiteral("network")));
        QVERIFY(names.contains(QStringLiteral("current_time")));
        QVERIFY(names.contains(QStringLiteral("image_info")));
    }

    void noServerAdvertisesCoreTools()
    {
        const QStringList core = {
            QStringLiteral("create_file"), QStringLiteral("create_directory"),
            QStringLiteral("read_file"), QStringLiteral("replace_string_in_file"),
            QStringLiteral("multi_replace_string_in_file"), QStringLiteral("file_search"),
            QStringLiteral("list_dir"), QStringLiteral("get_changed_files"),
            QStringLiteral("grep_search"), QStringLiteral("semantic_search"),
        };

        GithubToolServer github;
        WebToolServer web;
        BrowserToolServer browser;
        PylanceToolServer pylance;
        SonarQubeToolServer sonarqube;
        DatabaseToolServer database;
        SystemToolServer system;

        const QList<InProcessMcpServer *> servers = {
            &github, &web, &browser, &pylance, &sonarqube, &database, &system,
        };
        for (InProcessMcpServer *server : servers) {
            for (const QString &name : core)
                QVERIFY2(!server->hasTool(name),
                         qPrintable(QStringLiteral("server '%1' advertises core tool '%2'")
                                        .arg(server->name(), name)));
        }
    }
};

QTEST_MAIN(TestMcpToolServerGrouping)
#include "test_mcptoolservergrouping.moc"