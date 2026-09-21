#pragma once

// ── PKCE helpers ─────────────────────────────────────────────────────────────
//
// Header-only so both the `byok` plugin DSO and the host tests can use them
// without a link dependency. Used by the OpenRouter OAuth flow.

#include <QCryptographicHash>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

namespace Pkce {

/// S256 code challenge: base64url(SHA-256(verifier)) without padding.
inline QString challenge(const QString &verifier)
{
    const QByteArray digest = QCryptographicHash::hash(
        verifier.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

/// Localhost callback URL for the OpenRouter flow.
inline QString openRouterCallbackUrl(quint16 port)
{
    return QStringLiteral("http://localhost:%1/callback").arg(port);
}

/// OpenRouter authorization URL carrying the PKCE challenge and callback.
inline QUrl openRouterAuthUrl(const QString &callbackUrl, const QString &codeChallenge)
{
    QUrl url(QStringLiteral("https://openrouter.ai/auth"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("callback_url"), callbackUrl);
    q.addQueryItem(QStringLiteral("code_challenge"), codeChallenge);
    q.addQueryItem(QStringLiteral("code_challenge_method"), QStringLiteral("S256"));
    url.setQuery(q);
    return url;
}

} // namespace Pkce