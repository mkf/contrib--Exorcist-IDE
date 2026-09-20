#include <QTest>
#include <QLabel>
#include <QStatusBar>

#include "authstatusindicator.h"
#include "chatwelcomewidget.h"

// Guards the provider-neutral AI assistant branding: the generic surfaces must
// expose "Exorcist AI" / "AI Assistant" and must never display "Copilot".
class TestAiBranding : public QObject
{
    Q_OBJECT

private slots:
    void authIndicatorUsesNeutralName()
    {
        QStatusBar bar;
        AuthStatusIndicator indicator(&bar);

        const AuthStatusIndicator::State states[] = {
            AuthStatusIndicator::SignedOut,
            AuthStatusIndicator::SigningIn,
            AuthStatusIndicator::SignedIn,
            AuthStatusIndicator::Expired,
            AuthStatusIndicator::RateLimited,
            AuthStatusIndicator::Offline,
        };

        for (auto state : states) {
            indicator.setState(state);
            const QString text = indicator.label()->text();
            QVERIFY2(text.contains(QStringLiteral("Exorcist AI")),
                     qPrintable(QStringLiteral("state label missing 'Exorcist AI': ") + text));
            QVERIFY2(!text.contains(QStringLiteral("Copilot")),
                     qPrintable(QStringLiteral("state label still says 'Copilot': ") + text));
        }
    }

    void welcomeDefaultHeadingIsNeutral()
    {
        ChatWelcomeWidget welcome;
        welcome.showState(ChatWelcomeWidget::State::Default);

        const QStringList texts = labelTexts(welcome);
        QVERIFY2(texts.contains(QStringLiteral("Ask AI Assistant")),
                 "default welcome heading should read 'Ask AI Assistant'");
        QVERIFY2(!anyContains(texts, QStringLiteral("Copilot")),
                 "default welcome must not contain 'Copilot'");
    }

    void welcomeAuthPromptIsNeutral()
    {
        ChatWelcomeWidget welcome;
        welcome.showState(ChatWelcomeWidget::State::AuthRequired);

        const QStringList texts = labelTexts(welcome);
        QVERIFY2(texts.contains(QStringLiteral("Sign in to use the AI assistant.")),
                 "auth prompt should read 'Sign in to use the AI assistant.'");
        QVERIFY2(!anyContains(texts, QStringLiteral("Copilot")),
                 "auth prompt must not contain 'Copilot'");
    }

    void welcomeStatesNeverSayCopilot()
    {
        const ChatWelcomeWidget::State states[] = {
            ChatWelcomeWidget::State::Default,
            ChatWelcomeWidget::State::AuthRequired,
            ChatWelcomeWidget::State::RateLimited,
            ChatWelcomeWidget::State::Offline,
            ChatWelcomeWidget::State::NoProvider,
        };

        for (auto state : states) {
            ChatWelcomeWidget welcome;
            welcome.showState(state);
            QVERIFY2(!anyContains(labelTexts(welcome), QStringLiteral("Copilot")),
                     "welcome state must not contain 'Copilot'");
        }
    }

private:
    static QStringList labelTexts(const QWidget &widget)
    {
        QStringList texts;
        const auto labels = widget.findChildren<QLabel *>();
        for (const auto *label : labels)
            texts << label->text();
        return texts;
    }

    static bool anyContains(const QStringList &texts, const QString &needle)
    {
        for (const auto &text : texts) {
            if (text.contains(needle))
                return true;
        }
        return false;
    }
};

QTEST_MAIN(TestAiBranding)
#include "test_aibranding.moc"