#pragma once
// Minimal ZIP reader/writer with WinZip-compatible AES-256 encryption (AE-2, entries stored without compression).
// Opens with 7-Zip / WinZip / Keka / pyzipper using the password. No third-party code: only OpenSSL primitives.
// Reader limits: AES-encrypted entries with method 0 (stored) only, no ZIP64 - exactly what the writer produces.
#include "crypto.h"
#include "vault.h"
#include <QList>
#include <QString>

namespace tv::zip {

struct Entry {
    QString name;        // UTF-8, '/' separated
    QByteArray data;
};

Result writeAesZip(const QString &path, const QList<Entry> &entries, const QString &password);
Result readAesZip(const QString &path, const QString &password, QList<Entry> *out);

// Result::error for these two is exactly this marker so callers can tell "wrong password" apart.
inline QString wrongPasswordMarker() { return QStringLiteral("WRONG_PASSWORD"); }

} // namespace tv::zip
