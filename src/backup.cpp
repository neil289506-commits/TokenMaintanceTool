#include "backup.h"
#include "totp.h"
#include <QMap>
#include <QSet>
#include <openssl/crypto.h>

namespace tv {

namespace {

constexpr char kIniMagic[] = "TVINI1";
constexpr char kRsaMagic[] = "TVRSA1";
constexpr char kPassMagic[] = "TVPASS1";

// ---- tiny INI (key=value; values escape \ \n \r) ------------------------------------------------
QString esc(const QString &v)
{
    QString o;
    for (QChar c : v) {
        if (c == QLatin1Char('\\')) o += QLatin1String("\\\\");
        else if (c == QLatin1Char('\n')) o += QLatin1String("\\n");
        else if (c == QLatin1Char('\r')) o += QLatin1String("\\r");
        else o += c;
    }
    return o;
}
QString unesc(const QString &v)
{
    QString o;
    for (int i = 0; i < v.size(); ++i) {
        if (v[i] == QLatin1Char('\\') && i + 1 < v.size()) {
            const QChar n = v[++i];
            o += n == QLatin1Char('n') ? QLatin1Char('\n') : n == QLatin1Char('r') ? QLatin1Char('\r') : n;
        } else o += v[i];
    }
    return o;
}

struct Section {
    QString name;
    QMap<QString, QString> kv;
};

QByteArray writeIni(const QList<Section> &secs)
{
    QString s;
    for (const Section &sec : secs) {
        s += QLatin1Char('[') + sec.name + QStringLiteral("]\n");
        for (auto it = sec.kv.begin(); it != sec.kv.end(); ++it) s += it.key() + QLatin1Char('=') + esc(it.value()) + QLatin1Char('\n');
        s += QLatin1Char('\n');
    }
    return s.toUtf8();
}

QList<Section> parseIni(const QByteArray &data)
{
    QList<Section> out;
    for (const QString &raw : QString::fromUtf8(data).split(QLatin1Char('\n'))) {
        const QString line = raw.endsWith(QLatin1Char('\r')) ? raw.chopped(1) : raw;
        if (line.isEmpty() || line.startsWith(QLatin1Char(';'))) continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            out.append(Section{line.mid(1, line.size() - 2), {}});
        } else if (!out.isEmpty()) {
            const int eq = line.indexOf(QLatin1Char('='));
            if (eq > 0) out.last().kv.insert(line.left(eq), unesc(line.mid(eq + 1)));
        }
    }
    return out;
}

QString dt(const QDateTime &d) { return d.isValid() ? d.toUTC().toString(Qt::ISODate) : QString(); }
QDateTime parseDt(const QString &s)
{
    if (s.isEmpty()) return {};
    QDateTime d = QDateTime::fromString(s, Qt::ISODate);
    d.setTimeSpec(Qt::UTC);
    return d;
}

// ---- file names -------------------------------------------------------------------------------------
QString uniquePath(const QString &prefix, const QString &base, const QString &ext, QSet<QString> &used)
{
    QString name = base + ext;
    for (int n = 2; used.contains((prefix + name).toLower()); ++n) name = QStringLiteral("%1 (%2)%3").arg(base).arg(n).arg(ext);
    used.insert((prefix + name).toLower());
    return prefix + name;
}

QByteArray encryptIni(const QString &path, const QByteArray &plain, const SecureBytes &key)
{
    return QByteArray(kIniMagic) + aesGcmSeal(key, plain, "TVINI1|" + path.toUtf8());
}

bool decryptIni(const QString &path, const QByteArray &blob, const SecureBytes &key, SecureBytes *plain)
{
    const int m = int(sizeof(kIniMagic)) - 1;
    if (!blob.startsWith(kIniMagic)) return false;
    return aesGcmOpen(key, blob.mid(m), "TVINI1|" + path.toUtf8(), plain);
}

QString hex(const QByteArray &b) { return QString::fromLatin1(b.toHex()); }

bool isPng(const QByteArray &b) { return b.size() > 8 && b.startsWith(QByteArray("\x89PNG\r\n\x1a\n", 8)); }

} // namespace

// ---------------------------------------------------------------------------------------------------
QString sanitizeArchiveName(const QString &name)
{
    QString s;
    for (QChar c : name) s += (c.unicode() < 0x20 || QStringLiteral("<>:\"/\\|?*").contains(c)) ? QLatin1Char('_') : c;
    s = s.trimmed();
    while (s.endsWith(QLatin1Char('.')) || s.endsWith(QLatin1Char(' '))) s.chop(1);
    if (s.size() > 80) {
        s = s.left(80);
        if (s.back().isHighSurrogate()) s.chop(1);
    }
    if (s.isEmpty() || s == QLatin1String(".") || s == QLatin1String("..")) s = QStringLiteral("_");
    static const QStringList reserved = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
                                         "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    if (reserved.contains(s.section(QLatin1Char('.'), 0, 0).toUpper())) s.prepend(QLatin1Char('_'));
    return s;
}

QString normalizeBackupKey(const QString &k)
{
    QString o;
    for (QChar c : k)
        if (!c.isSpace() && c != QLatin1Char('-')) o += c;
    return o;
}

QString formatBackupKey(const QString &k)
{
    QStringList parts;
    for (int i = 0; i < k.size(); i += 6) parts << k.mid(i, 6);
    return parts.join(QLatin1Char('-'));
}

PKey generateBackupRsa() { return rsaGenerate(kBackupRsaBits); }

Result writeBackup(const BackupData &d, EVP_PKEY *rsa, const QString &path, BackupKeys *keysOut)
{
    try {
        if (!rsa) return Result::fail(QObject::tr("缺少 RSA-8192 金鑰"));
        BackupKeys keys;
        keys.zipKey = randomId(kBackupKeyLen);
        keys.rsaKey = randomId(kBackupKeyLen);

        QList<zip::Entry> entries;
        QSet<QString> usedPaths{QStringLiteral("readme.txt"), QStringLiteral("rsa8192.key"), QStringLiteral("passcode.txt")};
        QList<Section> files;                              // passcode.txt [file.N] path/key

        auto addIni = [&](const QString &p, const QByteArray &plain) {
            SecureBytes key(randomBytes(kAesKeyLen));
            entries.append({p, encryptIni(p, plain, key)});
            Section s;
            s.name = QStringLiteral("file.%1").arg(files.size());
            s.kv["path"] = p;
            s.kv["key"] = hex(key.data());
            files.append(s);
        };

        entries.append({QStringLiteral("README.txt"),
                        QByteArray("TokenVault backup\n\n"
                                   "Import it from TokenVault (Settings > Import, or the first-run screen).\n"
                                   "You need KEY 1 (archive password) and KEY 2 (RSA key password).\n"
                                   "Do not recompress or edit this archive.\n")});

        // rsa8192.key : salt | AES-GCM( PBKDF2-SHA512(KEY 2) , private key )
        {
            const QByteArray salt = randomBytes(16);
            SecureBytes kek = pbkdf2Sha512(keys.rsaKey.toUtf8(), salt, 200000, kAesKeyLen);
            entries.append({QStringLiteral("rsa8192.key"),
                            QByteArray(kRsaMagic) + salt + aesGcmSeal(kek, privToDer(rsa).data(), kRsaMagic)});
        }

        for (const BackupGroup &g : d.groups) {
            const QString folder = uniquePath(QString(), sanitizeArchiveName(g.name), QString(), usedPaths);
            Section gs;
            gs.name = QStringLiteral("group");
            gs.kv["name"] = g.name;
            gs.kv["note"] = g.note;
            gs.kv["icon"] = g.icon;
            addIni(folder + QStringLiteral("/group.ini"), writeIni({gs}));
            usedPaths.insert((folder + QStringLiteral("/group.ini")).toLower());
            if (!g.image.isEmpty()) entries.append({folder + QStringLiteral("/group.png"), g.image});

            for (const BackupToken &t : g.tokens) {
                const QString p = uniquePath(folder + QLatin1Char('/'), sanitizeArchiveName(t.name), QStringLiteral(".ini"), usedPaths);
                Section ts;
                ts.name = QStringLiteral("token");
                ts.kv["name"] = t.name;
                ts.kv["note"] = t.note;
                ts.kv["created"] = dt(t.created);
                ts.kv["updated"] = dt(t.updated);
                ts.kv["expires"] = dt(t.expires);
                ts.kv["revoked"] = t.revoked ? QStringLiteral("1") : QStringLiteral("0");
                ts.kv["secret_b64"] = QString::fromLatin1(t.secret.data().toBase64());
                addIni(p, writeIni({ts}));
            }
        }

        // passcode.txt
        Section v;
        v.name = QStringLiteral("vault");
        v.kv["format"] = QStringLiteral("1");
        v.kv["mode"] = QString::number(int(d.mode));
        if (d.mode & AuthPassphrase) v.kv["passphrase"] = d.passphrase;
        if (d.mode & AuthKeyfile) v.kv["keyfile_sha512"] = hex(d.keyfileHash);
        if (d.mode & AuthTotp) v.kv["totp_secret_base32"] = totp::base32Encode(d.totpSecret.data());
        QList<Section> all{v};
        all += files;
        QByteArray plain = writeIni(all);
        SecureBytes cek(randomBytes(kAesKeyLen));
        const QByteArray wrap = rsaOaepEncrypt(rsa, cek.data());                        // RSA-8192 OAEP(SHA-256)
        const QByteArray blob = aesGcmSeal(cek, plain, kPassMagic);
        OPENSSL_cleanse(plain.data(), size_t(plain.size()));
        entries.append({QStringLiteral("passcode.txt"),
                        QByteArray(kPassMagic) + "\n" + wrap.toBase64() + "\n" + blob.toBase64() + "\n"});

        if (Result r = zip::writeAesZip(path, entries, keys.zipKey); !r) return r;
        if (keysOut) *keysOut = keys;
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result readBackup(const QString &path, const QString &zipKeyIn, const QString &rsaKeyIn, BackupData *out)
{
    try {
        const QString zipKey = normalizeBackupKey(zipKeyIn), rsaKey = normalizeBackupKey(rsaKeyIn);
        if (zipKey.isEmpty()) return Result::fail(QObject::tr("請輸入第一組金鑰（壓縮包）"));
        if (rsaKey.isEmpty()) return Result::fail(QObject::tr("請輸入第二組金鑰（RSA）"));
        QList<zip::Entry> list;
        if (Result r = zip::readAesZip(path, zipKey, &list); !r)
            return r.error == zip::wrongPasswordMarker() ? Result::fail(QObject::tr("第一組金鑰（壓縮包）不正確")) : r;
        QMap<QString, QByteArray> files;
        for (const zip::Entry &e : list) files.insert(e.name, e.data);
        const QString bad = QObject::tr("不是有效的備份壓縮包，或檔案已損毀");

        // RSA-8192 private key
        const QByteArray rk = files.value(QStringLiteral("rsa8192.key"));
        const int rm = int(sizeof(kRsaMagic)) - 1;
        if (!rk.startsWith(kRsaMagic) || rk.size() < rm + 16 + kGcmIvLen + kGcmTagLen) return Result::fail(bad);
        SecureBytes kek = pbkdf2Sha512(rsaKey.toUtf8(), rk.mid(rm, 16), 200000, kAesKeyLen);
        SecureBytes privDer;
        if (!aesGcmOpen(kek, rk.mid(rm + 16), kRsaMagic, &privDer)) return Result::fail(QObject::tr("第二組金鑰（RSA）不正確"));
        PKey rsa = privFromDer(privDer);
        if (!rsa) return Result::fail(bad);

        // passcode.txt
        const QList<QByteArray> lines = files.value(QStringLiteral("passcode.txt")).split('\n');
        if (lines.size() < 3 || lines[0] != kPassMagic) return Result::fail(bad);
        SecureBytes cek, plain;
        if (!rsaOaepDecrypt(rsa.get(), QByteArray::fromBase64(lines[1]), &cek)
            || !aesGcmOpen(cek, QByteArray::fromBase64(lines[2]), kPassMagic, &plain))
            return Result::fail(bad);
        const QList<Section> secs = parseIni(plain.data());
        if (secs.isEmpty() || secs[0].name != QLatin1String("vault") || secs[0].kv.value("format") != QLatin1String("1"))
            return Result::fail(QObject::tr("備份格式版本不支援"));

        BackupData d;
        const QMap<QString, QString> &v = secs[0].kv;
        const int mode = v.value("mode").toInt();
        if (mode < 1 || mode > 7) return Result::fail(bad);
        d.mode = AuthMode(mode);
        d.passphrase = v.value("passphrase");
        d.keyfileHash = QByteArray::fromHex(v.value("keyfile_sha512").toLatin1());
        if (mode & AuthTotp) {
            bool ok = false;
            d.totpSecret = SecureBytes(totp::base32Decode(v.value("totp_secret_base32"), &ok));
            if (!ok || d.totpSecret.size() != totp::kSecretBytes) return Result::fail(bad);
        }
        if (((mode & AuthPassphrase) && d.passphrase.isEmpty()) || ((mode & AuthKeyfile) && d.keyfileHash.size() != 64))
            return Result::fail(bad);

        // every .ini listed in passcode.txt, grouped by folder
        QMap<QString, BackupGroup> groups;          // folder -> group
        QMap<QString, bool> haveGroupIni;
        for (int i = 1; i < secs.size(); ++i) {
            const QString p = secs[i].kv.value("path");
            const QByteArray keyBytes = QByteArray::fromHex(secs[i].kv.value("key").toLatin1());
            const int slash = p.indexOf(QLatin1Char('/'));
            if (slash <= 0 || p.contains(QStringLiteral("..")) || p.startsWith(QLatin1Char('/')) || keyBytes.size() != kAesKeyLen)
                return Result::fail(bad);
            if (!files.contains(p)) return Result::fail(QObject::tr("壓縮包缺少檔案：%1").arg(p));
            SecureBytes ini;
            if (!decryptIni(p, files.value(p), SecureBytes(keyBytes), &ini))
                return Result::fail(QObject::tr("「%1」解密失敗，檔案可能已損毀或被竄改").arg(p));
            const QList<Section> body = parseIni(ini.data());
            if (body.isEmpty()) return Result::fail(bad);
            const QString folder = p.left(slash);
            BackupGroup &g = groups[folder];
            if (g.name.isEmpty()) g.name = folder;
            if (p.endsWith(QStringLiteral("/group.ini")) && body[0].name == QLatin1String("group")) {
                const auto &kv = body[0].kv;
                g.name = kv.value("name").isEmpty() ? folder : kv.value("name");
                g.note = kv.value("note");
                g.icon = kv.value("icon");
                haveGroupIni[folder] = true;
            } else if (body[0].name == QLatin1String("token")) {
                const auto &kv = body[0].kv;
                BackupToken t;
                t.name = kv.value("name");
                t.note = kv.value("note");
                t.created = parseDt(kv.value("created"));
                t.updated = parseDt(kv.value("updated"));
                t.expires = parseDt(kv.value("expires"));
                t.revoked = kv.value("revoked") == QLatin1String("1");
                t.secret = SecureBytes(QByteArray::fromBase64(kv.value("secret_b64").toLatin1()));
                if (t.name.trimmed().isEmpty() || t.secret.isEmpty()) return Result::fail(QObject::tr("「%1」的內容不完整").arg(p));
                g.tokens.append(t);
            }
        }
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            const QByteArray png = files.value(it.key() + QStringLiteral("/group.png"));
            if (isPng(png) && png.size() <= 4 * 1024 * 1024) it->image = png;
            d.groups.append(it.value());
        }
        *out = d;
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

} // namespace tv
