#pragma once
// RFC 6238 TOTP (HMAC-SHA1, 6 digits, 30 s) + RFC 4648 Base32. Interoperable with Google Authenticator,
// Microsoft Authenticator, Aegis, 1Password, etc.
#include <QByteArray>
#include <QString>

namespace tv::totp {

constexpr int kDigits = 6;
constexpr int kPeriod = 30;
constexpr int kSecretBytes = 20;     // 160 bit, the RFC 4226 recommendation

QByteArray newSecret();
QString base32Encode(const QByteArray &d);
QByteArray base32Decode(const QString &s, bool *ok = nullptr);   // tolerant of spaces / lower case / '=' padding

QString code(const QByteArray &secret, qint64 unixSeconds, int digits = kDigits, int period = kPeriod);
// Accepts the current step +/- `window` steps (clock drift). Constant-time over the window.
bool verify(const QByteArray &secret, const QString &input, qint64 unixSeconds, int window = 1);
int secondsLeft(qint64 unixSeconds, int period = kPeriod);

QString otpauthUri(const QString &issuer, const QString &account, const QByteArray &secret);

} // namespace tv::totp
