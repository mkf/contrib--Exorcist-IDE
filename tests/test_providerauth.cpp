#include <QTest>
#include <QToolButton>

#include "aiinterface.h"
#include "agentproviderregistry.h"
#include "chatwelcomewidget.h"

// Minimal provider that relies on the IAgentProvider auth defaults.
class StubProvider : public IAgentProvider
{
    Q_OBJECT
public:
    explicit StubProvider(const QString &id, QObject *parent = nullptr)
        : m_id(id)
    {
        if (parent)
            setParent(parent);
    }

    QString           id()           const override { return m_id; }
    QString           displayName()  const override { return m_id; }
    AgentCapabilities capabilities() const override { return AgentCapability::Chat; }
    bool              isAvailable()  const override { return m_available; }

    QStringList       availableModels() const override { return {}; }
    QString           currentModel()    const override { return {}; }
    void              setModel(const QString &) override {}

    void initialize() override { ++initializeCalls; }
    void shutdown()   override {}

    void sendRequest(const AgentRequest &) override {}
    void cancelRequest(const QString &)    override {}

    int initializeCalls = 0;
    int startAuthCalls  = 0;

private:
    QString m_id;
    bool    m_available = false;
};

class TestProviderAuth : public QObject
{
    Q_OBJECT

private slots:
    void authDefaultsAreNone()
    {
        StubProvider p(QStringLiteral("stub"));
        const ProviderAuthInfo info = p.authInfo();
        QCOMPARE(info.kind, AuthAction::None);
        QVERIFY(info.actionLabel.isEmpty());
        QVERIFY(info.actionUrl.isEmpty());
        QVERIFY(info.settingsCommandId.isEmpty());
    }

    void defaultStartAuthIsNoOp()
    {
        StubProvider p(QStringLiteral("stub"));
        p.startAuth(); // must not crash
        QCOMPARE(p.startAuthCalls, 0);
    }

    void activatingProviderDoesNotStartAuth()
    {
        AgentProviderRegistry registry;
        auto *p = new StubProvider(QStringLiteral("stub"));
        registry.registerProvider(p);

        registry.setActiveProvider(QStringLiteral("stub"));

        QCOMPARE(p->startAuthCalls, 0);
        QVERIFY(p->initializeCalls > 0);
    }

    void welcomeRendersSuppliedActionAndLink()
    {
        ChatWelcomeWidget welcome;
        welcome.showAuthRequired(QStringLiteral("Sign in with GitHub"));

        const QStringList texts = buttonTexts(welcome);
        QVERIFY2(texts.contains(QStringLiteral("Sign in with GitHub")),
                 "primary action label should be rendered");
        QVERIFY2(texts.contains(QStringLiteral("or open settings to paste your API key")),
                 "secondary settings link should be rendered");
    }

    void welcomeWithoutLabelShowsOnlyLink()
    {
        ChatWelcomeWidget welcome;
        welcome.showAuthRequired(QString());

        const QStringList texts = buttonTexts(welcome);
        QVERIFY2(texts.contains(QStringLiteral("or open settings to paste your API key")),
                 "secondary settings link should still be rendered");
        QCOMPARE(texts.size(), 1);
    }

    void welcomeEmitsDistinctSignals()
    {
        ChatWelcomeWidget welcome;
        welcome.showAuthRequired(QStringLiteral("Sign in with OpenRouter"));

        int authCalls = 0;
        int settingsCalls = 0;
        connect(&welcome, &ChatWelcomeWidget::authActionRequested,
                this, [&authCalls] { ++authCalls; });
        connect(&welcome, &ChatWelcomeWidget::settingsRequested,
                this, [&settingsCalls] { ++settingsCalls; });

        const auto buttons = welcome.findChildren<QToolButton *>();
        for (auto *b : buttons) {
            if (b->text() == QLatin1String("Sign in with OpenRouter"))
                b->click();
            else if (b->text() == QLatin1String("or open settings to paste your API key"))
                b->click();
        }

        QCOMPARE(authCalls, 1);
        QCOMPARE(settingsCalls, 1);
    }

    void defaultWelcomeHasNoAuthAction()
    {
        ChatWelcomeWidget welcome;
        welcome.showState(ChatWelcomeWidget::State::Default);

        const QStringList texts = buttonTexts(welcome);
        QVERIFY2(!texts.contains(QStringLiteral("or open settings to paste your API key")),
                 "auth action must not be shown when the provider is available");
    }

private:
    static QStringList buttonTexts(const QWidget &widget)
    {
        QStringList texts;
        const auto buttons = widget.findChildren<QToolButton *>();
        for (const auto *b : buttons)
            texts << b->text();
        return texts;
    }
};

QTEST_MAIN(TestProviderAuth)
#include "test_providerauth.moc"