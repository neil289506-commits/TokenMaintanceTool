#pragma once
// Encrypted backup archive ("export") and its restore ("import").
//
// Archive = ZIP, every entry AES-256 encrypted with KEY 1 (48 random A-Za-z0-9; WinZip AES, opens in 7-Zip):
//   README.txt
//   rsa8192.key          RSA-8192 private key, AES-256-GCM-encrypted with a key derived from KEY 2
//   passcode.txt         RSA-8192-OAEP (hybrid) encrypted: current passphrase, key file SHA-512, TOTP secret,
//                        and one random AES-256 key PER .ini file
//   <Group>/group.ini    group name / note / icon        (AES-256-GCM with its own key from passcode.txt)
//   <Group>/group.png    custom group picture, if any    (plain PNG; the zip layer protects it)
//   <Group>/<name>.ini   one token each                  (AES-256-GCM with its own key from passcode.txt)
#include "aeszip.h"
#include "vault.h"

namespace tv {

struct BackupKeys {
    QString zipKey;     // KEY 1: unlocks the archive
    QString rsaKey;     // KEY 2: unlocks the RSA-8192 private key inside it
};

constexpr int kBackupRsaBits = 8192;
constexpr int kBackupKeyLen = 48;

// Remove spaces / dashes users may add when typing or pasting a key.
QString normalizeBackupKey(const QString &k);
// Shows a key in groups of 6 for readability ("abcdef-ghijkl-...").
QString formatBackupKey(const QString &k);

// Slow (several seconds to a minute). Start it early on a worker thread.
PKey generateBackupRsa();

Result writeBackup(const BackupData &d, EVP_PKEY *rsa8192, const QString &path, BackupKeys *keysOut);
Result readBackup(const QString &path, const QString &zipKey, const QString &rsaKey, BackupData *out);

// exposed for tests
QString sanitizeArchiveName(const QString &name);

} // namespace tv
