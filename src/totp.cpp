#include "totp.h"
#include "crypto.h"
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QUrl>

namespace tv::totp {

QByteArray newSecret() { return randomBytes(kSecretBytes); }

QString base32Encode(const QByteArray &d)
{
    static const char A[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    QString out;
    quint32 buf = 0;
    int bits = 0;
    for (unsigned char c : d) {
        buf = (buf << 8) | c;
        bits += 8;
        while (bits >= 5) {
            out.append(QLatin1Char(A[(buf >> (bits - 5)) & 31]));
            bits -= 5;
        }
    }
    if (bits > 0) out.append(QLatin1Char(A[(buf << (5 - bits)) & 31]));
    return out;                                                      // no '=' padding (accepted by all apps)
}

QByteArray base32Decode(const QString &s, bool *ok)
{
    QByteArray out;
    quint32 buf = 0;
    int bits = 0;
    bool good = true;
    for (QChar qc : s) {
        const ushort c = qc.toUpper().unicode();
        if (c == ' ' || c == '-' || c == '=') continue;
        int v;
        if (c >= 'A' && c <= 'Z') v = c - 'A';
        else if (c >= '2' && c <= '7') v = c - '2' + 26;
        else { good = false; break; }
        buf = (buf << 5) | quint32(v);
        bits += 5;
        if (bits >= 8) {
            out.append(char((buf >> (bits - 8)) & 0xFF));
            bits -= 8;
        }
    }
    if (ok) *ok = good;
    return good ? out : QByteArray();
}

QString code(const QByteArray &secret, qint64 t, int digits, int period)
{
    const quint64 counter = quint64(t / period);
    QByteArray msg(8, 0);
    for (int i = 7; i >= 0; --i) msg[7 - i] = char((counter >> (8 * i)) & 0xFF);
    const QByteArray h = QMessageAuthenticationCode::hash(msg, secret, QCryptographicHash::Sha1);
    const int off = h.at(h.size() - 1) & 0x0F;
    const quint32 bin = ((quint32(quint8(h[off])) & 0x7F) << 24) | (quint32(quint8(h[off + 1])) << 16)
                        | (quint32(quint8(h[off + 2])) << 8) | quint32(quint8(h[off + 3]));
    quint32 mod = 1;
    for (int i = 0; i < digits; ++i) mod *= 10;
    return QString::number(bin % mod).rightJustified(digits, QLatin1Char('0'));
}

bool verify(const QByteArray &secret, const QString &input, qint64 t, int window)
{
    const QString in = input.simplified().remove(QLatin1Char(' '));
    if (in.size() != kDigits) return false;
    bool match = false;
    for (int w = -window; w <= window; ++w) {
        const QString c = code(secret, t + qint64(w) * kPeriod);
        int diff = 0;
        for (int i = 0; i < kDigits; ++i) diff |= c[i].unicode() ^ in[i].unicode();
        match |= (diff == 0);                                        // no early exit
    }
    return match;
}

int secondsLeft(qint64 t, int period) { return period - int(t % period); }

QString otpauthUri(const QString &issuer, const QString &account, const QByteArray &secret)
{
    // Plain concatenation on purpose: percent-encoded text ("%3A") must never go through QString::arg().
    const QString iss = QString::fromUtf8(QUrl::toPercentEncoding(issuer));
    const QString label = QString::fromUtf8(QUrl::toPercentEncoding(issuer + QLatin1Char(':') + account));
    return QStringLiteral("otpauth://totp/") + label + QStringLiteral("?secret=") + base32Encode(secret)
           + QStringLiteral("&issuer=") + iss + QStringLiteral("&algorithm=SHA1&digits=") + QString::number(kDigits)
           + QStringLiteral("&period=") + QString::number(kPeriod);
}

} // namespace tv::totp
