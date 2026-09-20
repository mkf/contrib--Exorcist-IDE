#include <QtTest/QtTest>

#include <QSettings>
#include <QTemporaryDir>

#include "agent/openaicompatpresets.h"
#include "byokprovider.h"

using namespace OpenAICompat;

class TestOpenAiCompatPresets : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 1.1 preset registry / defaults
    void presetDefaults();

    // 1.2 effective endpoint fallback
    void effectiveEndpointFallback();
    void endpointOverridePerPreset();

    // 1.3 host classification
    void classifyHost_data();
    void classifyHost();

    // 2.3 models URL derivation
    void deriveModelsUrl_data();
    void deriveModelsUrl();

    // 2.6 migration
    void migrateLegacyToCustom();
    void migrateLegacyToOpenAi();
    void migrateLegacyToOpenRouter();
    void migrateLegacyIdempotent();

    // 2.2 model selection persistence
    void modelSelectionSurvivesReload();

    // 2.4 availability reflects api key
    void availabilityRequiresKey();

    // 5.3 unreachable endpoint must not crash and must keep the persisted list
    void unreachableEndpointKeepsPersistedList();

private:
    QTemporaryDir m_dir;
};

void TestOpenAiCompatPresets::initTestCase()
{
    QVERIFY(m_dir.isValid());
    // Isolate QSettings used by ByokProvider into a temporary INI file.
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
}

void TestOpenAiCompatPresets::cleanupTestCase()
{
}

void TestOpenAiCompatPresets::presetDefaults()
{
    QCOMPARE(presets().size(), 4);

    QVERIFY(presetByKey(QStringLiteral("openai")));
    QCOMPARE(presetByKey(QStringLiteral("openai"))->defaultEndpoint,
             QStringLiteral("https://api.openai.com/v1/chat/completions"));
    QCOMPARE(presetByKey(QStringLiteral("zai"))->defaultEndpoint,
             QStringLiteral("https://api.z.ai/api/coding/paas/v4/chat/completions"));
    QCOMPARE(presetByKey(QStringLiteral("openrouter"))->defaultEndpoint,
             QStringLiteral("https://openrouter.ai/api/v1/chat/completions"));
    QCOMPARE(presetByKey(QStringLiteral("custom"))->defaultEndpoint, QString());

    // Stable provider ids
    QCOMPARE(presetByKey(QStringLiteral("openai"))->id,
             QStringLiteral("openai-compat:openai"));
    QCOMPARE(presetByKey(QStringLiteral("zai"))->id,
             QStringLiteral("openai-compat:zai"));
    QCOMPARE(presetByKey(QStringLiteral("openrouter"))->id,
             QStringLiteral("openai-compat:openrouter"));
    QCOMPARE(presetByKey(QStringLiteral("custom"))->id,
             QStringLiteral("openai-compat:custom"));

    // Default preset is Custom
    QCOMPARE(defaultPreset()->key, QStringLiteral("custom"));
}

void TestOpenAiCompatPresets::effectiveEndpointFallback()
{
    const Preset *openrouter = presetByKey(QStringLiteral("openrouter"));
    QVERIFY(openrouter);
    QCOMPARE(effectiveEndpoint(*openrouter, QString()),
             openrouter->defaultEndpoint);
    QCOMPARE(effectiveEndpoint(*openrouter, QStringLiteral("   ")),
             openrouter->defaultEndpoint);
    QCOMPARE(effectiveEndpoint(*openrouter, QStringLiteral("https://proxy.example/v1/chat/completions")),
             QStringLiteral("https://proxy.example/v1/chat/completions"));
}

void TestOpenAiCompatPresets::endpointOverridePerPreset()
{
    QTemporaryDir dir;
    QSettings s(dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);

    saveEndpoint(s, QStringLiteral("openai"), QStringLiteral("https://proxy.example/v1/chat/completions"));

    QCOMPARE(loadEndpoint(s, QStringLiteral("openai")),
             QStringLiteral("https://proxy.example/v1/chat/completions"));
    // Other preset unaffected -> falls back to its default.
    QCOMPARE(effectiveEndpoint(*presetByKey(QStringLiteral("openrouter")),
                               loadEndpoint(s, QStringLiteral("openrouter"))),
             presetByKey(QStringLiteral("openrouter"))->defaultEndpoint);
}

void TestOpenAiCompatPresets::classifyHost_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expected");

    QTest::newRow("openai") << QStringLiteral("https://api.openai.com/v1/chat/completions")
                            << QStringLiteral("openai");
    QTest::newRow("openrouter") << QStringLiteral("https://openrouter.ai/api/v1/chat/completions")
                                << QStringLiteral("openrouter");
    QTest::newRow("other") << QStringLiteral("https://api.z.ai/api/coding/paas/v4/chat/completions")
                           << QStringLiteral("custom");
    QTest::newRow("local") << QStringLiteral("http://localhost:11434/v1/chat/completions")
                           << QStringLiteral("custom");
}

void TestOpenAiCompatPresets::classifyHost()
{
    QFETCH(QString, url);
    QFETCH(QString, expected);
    QCOMPARE(OpenAICompat::classifyHost(url), expected);
}

void TestOpenAiCompatPresets::deriveModelsUrl_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("chat-completions")
        << QStringLiteral("https://host/v1/chat/completions")
        << QStringLiteral("https://host/v1/models");
    QTest::newRow("with-query")
        << QStringLiteral("https://host/v1/chat/completions?api-version=2024-01-01")
        << QStringLiteral("https://host/v1/models");
    QTest::newRow("trailing-slash")
        << QStringLiteral("https://host/v1/chat/completions/")
        << QStringLiteral("https://host/v1/models");
    QTest::newRow("empty")
        << QString()
        << QString();
}

void TestOpenAiCompatPresets::deriveModelsUrl()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QCOMPARE(OpenAICompat::deriveModelsUrl(input), expected);
}

void TestOpenAiCompatPresets::migrateLegacyToCustom()
{
    QTemporaryDir dir;
    QSettings s(dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);
    s.setValue(legacyEndpointKey(), QStringLiteral("http://localhost:8080/v1/chat/completions"));
    s.setValue(legacyApiKeyKey(), QStringLiteral("local-key"));

    migrateLegacy(s);

    QCOMPARE(loadActivePreset(s), QStringLiteral("custom"));
    QCOMPARE(loadEndpoint(s, QStringLiteral("custom")),
             QStringLiteral("http://localhost:8080/v1/chat/completions"));
    QCOMPARE(loadApiKey(s, QStringLiteral("custom")), QStringLiteral("local-key"));
    QVERIFY(s.value(migratedKey()).toBool());
}

void TestOpenAiCompatPresets::migrateLegacyToOpenAi()
{
    QTemporaryDir dir;
    QSettings s(dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);
    s.setValue(legacyEndpointKey(), QStringLiteral("https://api.openai.com/v1/chat/completions"));
    s.setValue(legacyApiKeyKey(), QStringLiteral("sk-openai"));

    migrateLegacy(s);

    QCOMPARE(loadActivePreset(s), QStringLiteral("openai"));
    QCOMPARE(loadEndpoint(s, QStringLiteral("openai")),
             QStringLiteral("https://api.openai.com/v1/chat/completions"));
    QCOMPARE(loadApiKey(s, QStringLiteral("openai")), QStringLiteral("sk-openai"));
}

void TestOpenAiCompatPresets::migrateLegacyToOpenRouter()
{
    QTemporaryDir dir;
    QSettings s(dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);
    s.setValue(legacyEndpointKey(), QStringLiteral("https://openrouter.ai/api/v1/chat/completions"));
    s.setValue(legacyApiKeyKey(), QStringLiteral("sk-or"));

    migrateLegacy(s);

    QCOMPARE(loadActivePreset(s), QStringLiteral("openrouter"));
    QCOMPARE(loadEndpoint(s, QStringLiteral("openrouter")),
             QStringLiteral("https://openrouter.ai/api/v1/chat/completions"));
    QCOMPARE(loadApiKey(s, QStringLiteral("openrouter")), QStringLiteral("sk-or"));
}

void TestOpenAiCompatPresets::migrateLegacyIdempotent()
{
    QTemporaryDir dir;
    QSettings s(dir.filePath(QStringLiteral("s.ini")), QSettings::IniFormat);
    s.setValue(legacyEndpointKey(), QStringLiteral("http://localhost:8080/v1/chat/completions"));
    s.setValue(legacyApiKeyKey(), QStringLiteral("local-key"));
    migrateLegacy(s);

    // User edits after migration.
    saveEndpoint(s, QStringLiteral("custom"), QStringLiteral("https://edited/v1/chat/completions"));
    saveApiKey(s, QStringLiteral("custom"), QStringLiteral("edited-key"));

    // Second run must not overwrite.
    migrateLegacy(s);

    QCOMPARE(loadEndpoint(s, QStringLiteral("custom")),
             QStringLiteral("https://edited/v1/chat/completions"));
    QCOMPARE(loadApiKey(s, QStringLiteral("custom")), QStringLiteral("edited-key"));
}

void TestOpenAiCompatPresets::modelSelectionSurvivesReload()
{
    {
        ByokProvider first(QStringLiteral("openai"));
        first.setModel(QStringLiteral("gpt-4o-mini"));
    }
    ByokProvider reloaded(QStringLiteral("openai"));
    QCOMPARE(reloaded.currentModel(), QStringLiteral("gpt-4o-mini"));
}

void TestOpenAiCompatPresets::availabilityRequiresKey()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    ByokProvider provider(QStringLiteral("openai"));
    // No key saved -> unavailable even though the endpoint default exists.
    provider.reloadConfig();
    QVERIFY(!provider.isAvailable());

    {
        QSettings s;
        saveApiKey(s, QStringLiteral("openai"), QStringLiteral("sk-test"));
    }
    provider.reloadConfig();
    QVERIFY(provider.isAvailable());
    QCOMPARE(provider.effectiveEndpoint(),
             QStringLiteral("https://api.openai.com/v1/chat/completions"));
}

void TestOpenAiCompatPresets::unreachableEndpointKeepsPersistedList()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    {
        QSettings s;
        saveEndpoint(s, QStringLiteral("custom"),
                     QStringLiteral("http://127.0.0.1:9/v1/chat/completions"));
        saveApiKey(s, QStringLiteral("custom"), QStringLiteral("test-key"));
        saveModels(s, QStringLiteral("custom"),
                   {QStringLiteral("persisted-a"), QStringLiteral("persisted-b")});
        saveModel(s, QStringLiteral("custom"), QStringLiteral("persisted-a"));
    }

    ByokProvider provider(QStringLiteral("custom"));
    provider.initialize(); // fetches from an unreachable endpoint

    QSignalSpy spy(&provider, &IAgentProvider::modelsChanged);
    QVERIFY(spy.isValid());

    // Give the failing request time to complete; a crash would abort the test.
    QTest::qWait(300);

    const QStringList models = provider.availableModels();
    QCOMPARE(models.size(), 2);
    QVERIFY(models.contains(QStringLiteral("persisted-a")));
    QVERIFY(models.contains(QStringLiteral("persisted-b")));
    QCOMPARE(provider.currentModel(), QStringLiteral("persisted-a"));
}

QTEST_MAIN(TestOpenAiCompatPresets)
#include "test_openaicompatpresets.moc"