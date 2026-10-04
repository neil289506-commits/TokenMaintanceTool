#pragma once
// Vault = on-disk store. No GUI dependencies so it can be unit-tested headless.
//
// Layout (see DESIGN.md for the reasoning):
//   <dir>/vault.json                  master header: KDF params, RSA public key,
//                                     RSA private key + index key wrapped by AES-256-GCM(KEK)
//   <dir>/Token/<GroupId>/_group.grp  group metadata  (AES-256-GCM with index key)
//   <dir>/Token/<GroupId>/<Id6>.tkn   token: metadata (index key) + secret (RSA-4096-OAEP + AES-256-GCM)
#include "crypto.h"
#include <QDateTime>
#include <QList>
#include <QString>

namespace tv {

// Authentication methods (bit flags). Passphrase and key file are *key material* (they derive the KEK).
// TOTP is an additional verification gate: its secret is stored encrypted under the KEK, so the 6-digit code
// can only be checked after the passphrase / key file were right.
enum AuthMode { AuthPassphrase = 1, AuthKeyfile = 2, AuthTotp = 4 };
inline AuthMode operator|(AuthMode a, AuthMode b) { return AuthMode(int(a) | int(b)); }

// Rules: at least one of passphrase / key file; using BOTH ("two-factor") requires TOTP as well.
// Returns an error message, or an empty string when the combination is allowed.
QString validateMode(AuthMode m);

struct Credentials {
    QString passphrase;   // used when mode includes passphrase
    QString keyfile;      // path; used when mode includes keyfile
    QString totpCode;     // 6 digits; used when mode includes TOTP
};

struct Result {
    bool ok = true;
    QString error;
    static Result success() { return {}; }
    static Result fail(const QString &e) { Result r; r.ok = false; r.error = e; return r; }
    explicit operator bool() const { return ok; }
};

struct GroupInfo {
    QString id;       // 6 random alnum chars = directory name
    QString name;
    QString note;
    QString icon;     // "<shape>:<colorIndex>", rendered by icons.cpp
};

struct TokenInfo {
    enum Status { Valid, Expired, Revoked };
    QString id;
    QString groupId;
    QString name;
    QString note;
    QDateTime created;
    QDateTime updated;
    QDateTime expires;   // invalid => never expires
    bool revoked = false;

    Status status(const QDateTime &now = QDateTime::currentDateTimeUtc()) const;
    // Whole days until expiry (floor); negative if expired; INT_MAX if never.
    int daysLeft(const QDateTime &now = QDateTime::currentDateTimeUtc()) const;
};

class Vault {
public:
    explicit Vault(const QString &baseDir);

    static QString defaultDir();             // honours $TOKENVAULT_DIR, else AppData location
    QString baseDir() const { return m_dir; }

    bool exists() const;
    AuthMode mode() const;                   // from vault.json (valid only if exists())

    // --- slow (PBKDF2 / RSA keygen): call from a worker thread -------------
    Result create(AuthMode mode, const Credentials &c, const QByteArray &totpSecret = QByteArray());   // leaves it unlocked
    Result unlock(const Credentials &c);
    Result verify(const Credentials &c) const;             // re-authentication, no state change
    // Needs the current credentials (incl. current TOTP code). If newMode has TOTP and newTotpSecret is empty,
    // the existing TOTP secret is kept (only possible when the old mode had TOTP).
    // `oldTotpAlreadyVerified`: the caller just passed a full verification (incl. TOTP) in the UI, so the
    // (possibly expired) code in oldC is not checked again; passphrase / key file are always re-checked.
    Result changeCredentials(const Credentials &oldC, AuthMode newMode, const Credentials &newC,
                             const QByteArray &newTotpSecret = QByteArray(), bool oldTotpAlreadyVerified = false);
    Result revealToken(const QString &groupId, const QString &id, const Credentials &c, SecureBytes *out) const;

    // "Forgot password": permanently deletes vault.json and every file under Token/. Nothing else in the
    // folder is touched. Needs no credentials, by design.
    Result wipeAll();

    void lock();
    bool isUnlocked() const { return !m_index.isEmpty() && m_pub; }

    // --- need only the in-memory session (public key + index key) ----------
    QList<GroupInfo> groups(QStringList *warnings = nullptr) const;
    QList<TokenInfo> tokens(const QString &groupId, QStringList *warnings = nullptr) const;

    Result createGroup(const QString &name, const QString &note, const QString &icon, QString *idOut);
    Result updateGroup(const GroupInfo &g);
    Result deleteGroup(const QString &id);                 // removes all tokens in it

    Result addToken(const QString &groupId, const QString &name, const QString &note,
                    const QDateTime &expires, const SecureBytes &token, QString *idOut);
    Result updateTokenMeta(const TokenInfo &t);            // name / note / expires / revoked
    Result replaceTokenSecret(const QString &groupId, const QString &id, const SecureBytes &token,
                              bool changeExpiry, const QDateTime &newExpires /*invalid => never*/, bool clearRevoked);
    Result deleteToken(const QString &groupId, const QString &id);

    // paths (public for tests)
    QString vaultFile() const;
    QString groupDir(const QString &gid) const;
    QString tokenFile(const QString &gid, const QString &id) const;

    static constexpr int kPbkdf2Iterations = 600000;

private:
    struct Header;
    bool readHeader(Header *h, QString *err) const;
    Result deriveAndOpen(const Credentials &c, const Header &h, SecureBytes *priv, SecureBytes *index,
                         SecureBytes *totpSecret = nullptr, bool checkTotp = true) const;
    Result writeVaultFile(AuthMode mode, const Credentials &c, EVP_PKEY *pub, const SecureBytes &privDer,
                          const SecureBytes &index, const SecureBytes &totpSecret) const;

    QString m_dir;
    PKey m_pub;
    SecureBytes m_index;
};

} // namespace tv
