#include <QTest>
#include <QUrlQuery>

#include "pkce.h"

// Guards the OpenRouter OAuth PKCE helpers: S256 challenge derivation and the
// authorization/callback URL construction.
class TestPkce : public QObject
{
    Q_OBJECT

private slots:
    void challengeMatchesRfc7636Vector()
    {
        // RFC 7636 Appendix B test vector.
        const QString verifier = QStringLiteral("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk");
        QCOMPARE(Pkce::challenge(verifier),
                 QStringLiteral("E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"));
    }

    void challengeIsBase64UrlWithoutPadding()
    {
        const QString c = Pkce::challenge(QStringLiteral("some-verifier-value"));
        QVERIFY(!c.contains(QLatin1Char('+')));
        QVERIFY(!c.contains(QLatin1Char('/')));
        QVERIFY(!c.contains(QLatin1Char('=')));
    }

    void callbackUrlUsesLocalhostPort()
    {
        QCOMPARE(Pkce::openRouterCallbackUrl(12345),
                 QStringLiteral("http://localhost:12345/callback"));
    }

    void authUrlCarriesPkceParameters()
    {
        const QString callback = Pkce::openRouterCallbackUrl(54321);
        const QString challenge = Pkce::challenge(QStringLiteral("verifier"));
        const QUrl url = Pkce::openRouterAuthUrl(callback, challenge);

        QCOMPARE(url.scheme(), QStringLiteral("https"));
        QCOMPARE(url.host(), QStringLiteral("openrouter.ai"));
        QCOMPARE(url.path(), QStringLiteral("/auth"));

        const QUrlQuery q(url);
        QCOMPARE(q.queryItemValue(QStringLiteral("callback_url")), callback);
        QCOMPARE(q.queryItemValue(QStringLiteral("code_challenge")), challenge);
        QCOMPARE(q.queryItemValue(QStringLiteral("code_challenge_method")),
                 QStringLiteral("S256"));
    }
};

QTEST_MAIN(TestPkce)
#include "test_pkce.moc"