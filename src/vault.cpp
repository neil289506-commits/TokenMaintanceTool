#include "vault.h"
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <openssl/crypto.h>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtGlobal>
#include <climits>
#include <stdexcept>

namespace tv {

namespace {

constexpr int kVersion = 1;

QByteArray b64(const QByteArray &d) { return d.toBase64(); }
QByteArray unb64(const QJsonValue &v) { return QByteArray::fromBase64(v.toString().toLatin1()); }

QString dtToStr(const QDateTime &d) { return d.isValid() ? d.toUTC().toString(Qt::ISODate) : QString(); }
QDateTime strToDt(const QString &s)
{
    if (s.isEmpty()) return {};
    QDateTime d = QDateTime::fromString(s, Qt::ISODate);
    d.setTimeSpec(Qt::UTC);
    return d;
}

bool readJson(const QString &path, QJsonObject *o)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !d.isObject()) return false;
    *o = d.object();
    return true;
}

bool writeJsonAtomic(const QString &path, const QJsonObject &o, QString *err)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) { if (err) *err = f.errorString(); return false; }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    if (!f.commit()) { if (err) *err = f.errorString(); return false; }
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

// Best-effort overwrite before unlink. (SSD wear levelling / CoW filesystems may keep old blocks;
// the data is encrypted anyway, this is only defence in depth.)
void secureRemove(const QString &path)
{
    QFile f(path);
    if (f.exists() && f.open(QIODevice::ReadWrite)) {
        const qint64 n = f.size();
        f.seek(0);
        f.write(randomBytes(int(qMin<qint64>(n, 1 << 20))));
        f.flush();
        f.close();
    }
    QFile::remove(path);
}

QByteArray aadFor(const char *label, const QString &id) { return QByteArray(label) + '|' + id.toUtf8(); }

} // namespace

// ---------------------------------------------------------------------------
TokenInfo::Status TokenInfo::status(const QDateTime &now) const
{
    if (revoked) return Revoked;
    if (expires.isValid() && expires <= now) return Expired;
    return Valid;
}

int TokenInfo::daysLeft(const QDateTime &now) const
{
    if (!expires.isValid()) return INT_MAX;
    const qint64 secs = now.secsTo(expires);
    return int(secs >= 0 ? secs / 86400 : -((-secs + 86399) / 86400));
}

// ---------------------------------------------------------------------------
struct Vault::Header {
    AuthMode mode = AuthPassphrase;
    int iterations = 0;
    QByteArray salt;
    QByteArray pub;
    QByteArray encPriv;
    QByteArray encIndex;
    QByteArray aadSuffix() const
    {
        return QByteArray("|m") + QByteArray::number(int(mode)) + "|i" + QByteArray::number(iterations) + "|s" + salt.toHex();
    }
};

Vault::Vault(const QString &baseDir) : m_dir(baseDir) {}

QString Vault::defaultDir()
{
    const QByteArray env = qgetenv("TOKENVAULT_DIR");
    if (!env.isEmpty()) return QString::fromLocal8Bit(env);
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Vault::vaultFile() const { return m_dir + QStringLiteral("/vault.json"); }
QString Vault::groupDir(const QString &gid) const { return m_dir + QStringLiteral("/Token/") + gid; }
QString Vault::tokenFile(const QString &gid, const QString &id) const { return groupDir(gid) + QLatin1Char('/') + id + QStringLiteral(".tkn"); }

bool Vault::exists() const { return QFile::exists(vaultFile()); }

bool Vault::readHeader(Header *h, QString *err) const
{
    QJsonObject o;
    if (!readJson(vaultFile(), &o)) { if (err) *err = QObject::tr("無法讀取 vault.json"); return false; }
    if (o.value("v").toInt() != kVersion) { if (err) *err = QObject::tr("不支援的保險庫版本"); return false; }
    const int m = o.value("mode").toInt();
    if (m < 1 || m > 3) { if (err) *err = QObject::tr("保險庫檔案已損毀"); return false; }
    h->mode = AuthMode(m);
    const QJsonObject kdf = o.value("kdf").toObject();
    h->iterations = kdf.value("iter").toInt();
    h->salt = unb64(kdf.value("salt"));
    h->pub = unb64(o.value("pub"));
    h->encPriv = unb64(o.value("encPriv"));
    h->encIndex = unb64(o.value("encIndex"));
    if (h->iterations < 100000 || h->salt.size() < 16 || h->pub.isEmpty() || h->encPriv.isEmpty() || h->encIndex.isEmpty()) {
        if (err) *err = QObject::tr("保險庫檔案已損毀");
        return false;
    }
    return true;
}

AuthMode Vault::mode() const
{
    Header h;
    QString e;
    return readHeader(&h, &e) ? h.mode : AuthPassphrase;
}

// Material fed to PBKDF2: "TV1" | mode | len(pass) | pass | SHA-512(keyfile)
static Result buildMaterial(AuthMode mode, const Credentials &c, SecureBytes *out)
{
    QByteArray m("TV1");
    m.append(char(mode));
    if (mode & AuthPassphrase) {
        const QByteArray p = c.passphrase.toUtf8();
        if (p.isEmpty()) return Result::fail(QObject::tr("請輸入密碼"));
        const quint32 n = quint32(p.size());
        for (int i = 3; i >= 0; --i) m.append(char((n >> (8 * i)) & 0xFF));
        m.append(p);
    }
    if (mode & AuthKeyfile) {
        if (c.keyfile.isEmpty()) return Result::fail(QObject::tr("請選擇鑰匙檔"));
        QString err;
        const QByteArray h = sha512File(c.keyfile, &err);
        if (h.isEmpty()) return Result::fail(QObject::tr("無法讀取鑰匙檔：%1").arg(err));
        m.append(h);
    }
    *out = SecureBytes(m);
    OPENSSL_cleanse(m.data(), size_t(m.size()));
    return Result::success();
}

Result Vault::deriveAndOpen(const Credentials &c, const Header &h, SecureBytes *priv, SecureBytes *index) const
{
    SecureBytes material;
    if (Result r = buildMaterial(h.mode, c, &material); !r) return r;
    SecureBytes kdf = pbkdf2Sha512(material.data(), h.salt, h.iterations, 64);
    SecureBytes kek(kdf.data().left(kAesKeyLen));
    const QByteArray suffix = h.aadSuffix();
    SecureBytes p, i;
    if (!aesGcmOpen(kek, h.encPriv, "TV1|priv" + suffix, &p) || !aesGcmOpen(kek, h.encIndex, "TV1|idx" + suffix, &i))
        return Result::fail(QObject::tr("密碼或鑰匙檔不正確"));
    if (i.size() != kAesKeyLen) return Result::fail(QObject::tr("保險庫檔案已損毀"));
    if (priv) *priv = p;
    if (index) *index = i;
    return Result::success();
}

Result Vault::writeVaultFile(AuthMode mode, const Credentials &c, EVP_PKEY *pub,
                             const SecureBytes &privDer, const SecureBytes &index) const
{
    Header h;
    h.mode = mode;
    h.iterations = kPbkdf2Iterations;
    h.salt = randomBytes(32);
    SecureBytes material;
    if (Result r = buildMaterial(mode, c, &material); !r) return r;
    SecureBytes kdf = pbkdf2Sha512(material.data(), h.salt, h.iterations, 64);
    SecureBytes kek(kdf.data().left(kAesKeyLen));
    const QByteArray suffix = h.aadSuffix();

    QJsonObject o;
    o["v"] = kVersion;
    o["mode"] = int(mode);
    QJsonObject kdfo;
    kdfo["alg"] = "PBKDF2-HMAC-SHA512";
    kdfo["iter"] = h.iterations;
    kdfo["salt"] = QString::fromLatin1(b64(h.salt));
    o["kdf"] = kdfo;
    o["pub"] = QString::fromLatin1(b64(pubToDer(pub)));
    o["encPriv"] = QString::fromLatin1(b64(aesGcmSeal(kek, privDer.data(), "TV1|priv" + suffix)));
    o["encIndex"] = QString::fromLatin1(b64(aesGcmSeal(kek, index.data(), "TV1|idx" + suffix)));
    QString err;
    if (!writeJsonAtomic(vaultFile(), o, &err)) return Result::fail(err);
    return Result::success();
}

Result Vault::create(AuthMode mode, const Credentials &c)
{
    if (exists()) return Result::fail(QObject::tr("保險庫已存在"));
    try {
        QDir().mkpath(m_dir);
        PKey key = rsaGenerate();
        SecureBytes priv = privToDer(key.get());
        SecureBytes index(randomBytes(kAesKeyLen));
        if (Result r = writeVaultFile(mode, c, key.get(), priv, index); !r) return r;
        m_pub = pubFromDer(pubToDer(key.get()));
        m_index = index;
        QDir().mkpath(m_dir + QStringLiteral("/Token"));
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::unlock(const Credentials &c)
{
    Header h;
    QString err;
    if (!readHeader(&h, &err)) return Result::fail(err);
    try {
        SecureBytes idx;
        if (Result r = deriveAndOpen(c, h, nullptr, &idx); !r) return r;
        PKey pub = pubFromDer(h.pub);
        if (!pub) return Result::fail(QObject::tr("保險庫檔案已損毀"));
        m_pub = std::move(pub);
        m_index = idx;
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::verify(const Credentials &c) const
{
    Header h;
    QString err;
    if (!readHeader(&h, &err)) return Result::fail(err);
    try {
        return deriveAndOpen(c, h, nullptr, nullptr);
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::changeCredentials(const Credentials &oldC, AuthMode newMode, const Credentials &newC)
{
    Header h;
    QString err;
    if (!readHeader(&h, &err)) return Result::fail(err);
    try {
        SecureBytes priv, idx;
        if (Result r = deriveAndOpen(oldC, h, &priv, &idx); !r) return r;
        PKey pub = pubFromDer(h.pub);
        if (!pub) return Result::fail(QObject::tr("保險庫檔案已損毀"));
        // Same RSA pair and index key => no token file needs rewriting.
        return writeVaultFile(newMode, newC, pub.get(), priv, idx);
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

void Vault::lock()
{
    m_index.wipe();
    m_pub.reset();
}

// --- groups ------------------------------------------------------------------

QList<GroupInfo> Vault::groups(QStringList *warnings) const
{
    QList<GroupInfo> out;
    if (!isUnlocked()) return out;
    const QDir root(m_dir + QStringLiteral("/Token"));
    for (const QString &gid : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
        QJsonObject o;
        SecureBytes plain;
        if (!readJson(root.filePath(gid + "/_group.grp"), &o)
            || !aesGcmOpen(m_index, unb64(o.value("meta")), aadFor("GRP1", gid), &plain)) {
            if (warnings) warnings->append(QObject::tr("群組 %1 無法讀取或已被竄改").arg(gid));
            continue;
        }
        const QJsonObject m = QJsonDocument::fromJson(plain.data()).object();
        GroupInfo g;
        g.id = gid;
        g.name = m.value("name").toString();
        g.note = m.value("note").toString();
        g.icon = m.value("icon").toString();
        out.append(g);
    }
    return out;
}

static Result writeGroup(const Vault &v, const SecureBytes &index, const GroupInfo &g)
{
    QJsonObject m;
    m["name"] = g.name;
    m["note"] = g.note;
    m["icon"] = g.icon;
    QJsonObject o;
    o["v"] = kVersion;
    o["meta"] = QString::fromLatin1(b64(aesGcmSeal(index, QJsonDocument(m).toJson(QJsonDocument::Compact), aadFor("GRP1", g.id))));
    QString err;
    if (!writeJsonAtomic(v.groupDir(g.id) + "/_group.grp", o, &err)) return Result::fail(err);
    return Result::success();
}

Result Vault::createGroup(const QString &name, const QString &note, const QString &icon, QString *idOut)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    if (name.trimmed().isEmpty()) return Result::fail(QObject::tr("請輸入群組名稱"));
    try {
        QString id;
        do { id = randomId(6); } while (QDir(groupDir(id)).exists());
        GroupInfo g{id, name.trimmed(), note, icon};
        QDir().mkpath(groupDir(id));
        if (Result r = writeGroup(*this, m_index, g); !r) return r;
        if (idOut) *idOut = id;
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::updateGroup(const GroupInfo &g)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    if (g.name.trimmed().isEmpty()) return Result::fail(QObject::tr("請輸入群組名稱"));
    try {
        GroupInfo c = g;
        c.name = g.name.trimmed();
        return writeGroup(*this, m_index, c);
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::deleteGroup(const QString &id)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    if (id.contains('/') || id.contains('\\') || id.contains("..")) return Result::fail(QObject::tr("無效的群組"));
    const QDir d(groupDir(id));
    for (const QString &f : d.entryList(QDir::Files)) secureRemove(d.filePath(f));
    if (!QDir().rmdir(d.path()) && d.exists()) return Result::fail(QObject::tr("無法刪除群組資料夾"));
    return Result::success();
}

// --- tokens ------------------------------------------------------------------

namespace {
QByteArray metaJson(const TokenInfo &t)
{
    QJsonObject m;
    m["name"] = t.name;
    m["note"] = t.note;
    m["created"] = dtToStr(t.created);
    m["updated"] = dtToStr(t.updated);
    m["expires"] = dtToStr(t.expires);
    m["revoked"] = t.revoked;
    return QJsonDocument(m).toJson(QJsonDocument::Compact);
}
}

QList<TokenInfo> Vault::tokens(const QString &groupId, QStringList *warnings) const
{
    QList<TokenInfo> out;
    if (!isUnlocked()) return out;
    const QDir d(groupDir(groupId));
    for (const QString &fn : d.entryList({QStringLiteral("*.tkn")}, QDir::Files, QDir::Name)) {
        const QString id = fn.left(fn.size() - 4);
        QJsonObject o;
        SecureBytes plain;
        if (!readJson(d.filePath(fn), &o) || !aesGcmOpen(m_index, unb64(o.value("meta")), aadFor("TKN-META", id), &plain)) {
            if (warnings) warnings->append(QObject::tr("Token %1/%2 無法讀取或已被竄改").arg(groupId, id));
            continue;
        }
        const QJsonObject m = QJsonDocument::fromJson(plain.data()).object();
        TokenInfo t;
        t.id = id;
        t.groupId = groupId;
        t.name = m.value("name").toString();
        t.note = m.value("note").toString();
        t.created = strToDt(m.value("created").toString());
        t.updated = strToDt(m.value("updated").toString());
        t.expires = strToDt(m.value("expires").toString());
        t.revoked = m.value("revoked").toBool();
        out.append(t);
    }
    return out;
}

// Secret blob = SHA-256(token) | token, sealed with a fresh AES-256 key that is wrapped with RSA-4096-OAEP.
static QJsonObject sealSecret(EVP_PKEY *pub, const QString &id, const SecureBytes &token)
{
    SecureBytes cek(randomBytes(kAesKeyLen));
    QByteArray plain = sha256(token.data()) + token.data();
    QJsonObject o;
    o["wrap"] = QString::fromLatin1(b64(rsaOaepEncrypt(pub, cek.data())));
    o["secret"] = QString::fromLatin1(b64(aesGcmSeal(cek, plain, aadFor("TKN-SEC", id))));
    OPENSSL_cleanse(plain.data(), size_t(plain.size()));
    return o;
}

static Result writeToken(const Vault &v, const SecureBytes &index, const TokenInfo &t, const QJsonObject &secretPart)
{
    QJsonObject o = secretPart;
    o["v"] = kVersion;
    o["meta"] = QString::fromLatin1(b64(aesGcmSeal(index, metaJson(t), aadFor("TKN-META", t.id))));
    QString err;
    if (!writeJsonAtomic(v.tokenFile(t.groupId, t.id), o, &err)) return Result::fail(err);
    return Result::success();
}

Result Vault::addToken(const QString &groupId, const QString &name, const QString &note,
                       const QDateTime &expires, const SecureBytes &token, QString *idOut)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    if (name.trimmed().isEmpty()) return Result::fail(QObject::tr("請輸入名稱"));
    if (token.isEmpty()) return Result::fail(QObject::tr("請輸入 Token"));
    if (!QDir(groupDir(groupId)).exists()) return Result::fail(QObject::tr("群組不存在"));
    try {
        QString id;
        do { id = randomId(6); } while (QFile::exists(tokenFile(groupId, id)));
        TokenInfo t;
        t.id = id;
        t.groupId = groupId;
        t.name = name.trimmed();
        t.note = note;
        t.created = t.updated = QDateTime::currentDateTimeUtc();
        t.expires = expires;
        if (Result r = writeToken(*this, m_index, t, sealSecret(m_pub.get(), id, token)); !r) return r;
        if (idOut) *idOut = id;
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::updateTokenMeta(const TokenInfo &t)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    if (t.name.trimmed().isEmpty()) return Result::fail(QObject::tr("請輸入名稱"));
    try {
        QJsonObject o;
        if (!readJson(tokenFile(t.groupId, t.id), &o)) return Result::fail(QObject::tr("Token 檔案不存在"));
        TokenInfo c = t;
        c.name = t.name.trimmed();
        c.updated = QDateTime::currentDateTimeUtc();
        QJsonObject secretPart;
        secretPart["wrap"] = o.value("wrap");
        secretPart["secret"] = o.value("secret");
        return writeToken(*this, m_index, c, secretPart);
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::replaceTokenSecret(const QString &groupId, const QString &id, const SecureBytes &token,
                                 bool changeExpiry, const QDateTime &newExpires, bool clearRevoked)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    if (token.isEmpty()) return Result::fail(QObject::tr("請輸入 Token"));
    try {
        const QList<TokenInfo> all = tokens(groupId);
        for (TokenInfo t : all) {
            if (t.id != id) continue;
            if (changeExpiry) t.expires = newExpires;
            if (clearRevoked) t.revoked = false;
            t.updated = QDateTime::currentDateTimeUtc();
            return writeToken(*this, m_index, t, sealSecret(m_pub.get(), id, token));
        }
        return Result::fail(QObject::tr("Token 不存在"));
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result Vault::deleteToken(const QString &groupId, const QString &id)
{
    if (!isUnlocked()) return Result::fail(QObject::tr("保險庫已鎖定"));
    const QString p = tokenFile(groupId, id);
    if (!QFile::exists(p)) return Result::fail(QObject::tr("Token 不存在"));
    secureRemove(p);
    return QFile::exists(p) ? Result::fail(QObject::tr("無法刪除檔案")) : Result::success();
}

Result Vault::revealToken(const QString &groupId, const QString &id, const Credentials &c, SecureBytes *out) const
{
    Header h;
    QString err;
    if (!readHeader(&h, &err)) return Result::fail(err);
    try {
        SecureBytes privDer;
        if (Result r = deriveAndOpen(c, h, &privDer, nullptr); !r) return r;
        PKey priv = privFromDer(privDer);
        if (!priv) return Result::fail(QObject::tr("保險庫檔案已損毀"));
        QJsonObject o;
        if (!readJson(tokenFile(groupId, id), &o)) return Result::fail(QObject::tr("Token 檔案不存在"));
        SecureBytes cek, plain;
        if (!rsaOaepDecrypt(priv.get(), unb64(o.value("wrap")), &cek)
            || !aesGcmOpen(cek, unb64(o.value("secret")), aadFor("TKN-SEC", id), &plain)
            || plain.size() < 32)
            return Result::fail(QObject::tr("Token 解密失敗（檔案可能已損毀或被竄改）"));
        const QByteArray tok = plain.data().mid(32);
        if (sha256(tok) != plain.data().left(32))
            return Result::fail(QObject::tr("Token 完整性檢查 (SHA-256) 失敗"));
        *out = SecureBytes(tok);
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

} // namespace tv
