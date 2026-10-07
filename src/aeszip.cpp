#include "aeszip.h"
#include <QDateTime>
#include <QFile>
#include <QSaveFile>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdexcept>

namespace tv::zip {

namespace {

constexpr quint32 kLocalSig = 0x04034b50, kCentralSig = 0x02014b50, kEndSig = 0x06054b50;
constexpr quint16 kMethodAes = 99, kFlagEncrypted = 0x0001, kFlagUtf8 = 0x0800;
constexpr qint64 kMaxArchive = 512LL * 1024 * 1024;

inline const unsigned char *u(const QByteArray &b) { return reinterpret_cast<const unsigned char *>(b.constData()); }
inline unsigned char *u(QByteArray &b) { return reinterpret_cast<unsigned char *>(b.data()); }

void put16(QByteArray &b, quint16 v) { b.append(char(v & 0xFF)); b.append(char(v >> 8)); }
void put32(QByteArray &b, quint32 v) { for (int i = 0; i < 4; ++i) b.append(char((v >> (8 * i)) & 0xFF)); }
quint16 get16(const QByteArray &b, qint64 o) { return quint16(quint8(b[o]) | (quint8(b[o + 1]) << 8)); }
quint32 get32(const QByteArray &b, qint64 o) { return quint32(get16(b, o)) | (quint32(get16(b, o + 2)) << 16); }

struct AesParams { int keyLen, saltLen; };
AesParams paramsFor(int strength)
{
    switch (strength) {
    case 1: return {16, 8};
    case 2: return {24, 12};
    default: return {32, 16};
    }
}

// WinZip AES-CTR: AES-ECB over a LITTLE-endian 128-bit counter that starts at 1 (OpenSSL's CTR is big-endian,
// so we build the keystream ourselves).
QByteArray winzipCtr(const QByteArray &key, const QByteArray &in)
{
    const EVP_CIPHER *ci = key.size() == 16 ? EVP_aes_128_ecb() : key.size() == 24 ? EVP_aes_192_ecb() : EVP_aes_256_ecb();
    std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX *)> ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx || EVP_EncryptInit_ex(ctx.get(), ci, nullptr, u(key), nullptr) != 1) throw std::runtime_error("AES init failed");
    EVP_CIPHER_CTX_set_padding(ctx.get(), 0);
    QByteArray out(in.size(), 0);
    unsigned char ctr[16] = {0}, ks[32];
    for (qint64 off = 0; off < in.size(); off += 16) {
        for (int i = 0; i < 16 && ++ctr[i] == 0; ++i) {}          // little-endian increment, first block uses 1
        int n = 0;
        if (EVP_EncryptUpdate(ctx.get(), ks, &n, ctr, 16) != 1 || n != 16) throw std::runtime_error("AES block failed");
        const int len = int(qMin<qint64>(16, in.size() - off));
        for (int i = 0; i < len; ++i) out[off + i] = char(quint8(in[off + i]) ^ ks[i]);
    }
    return out;
}

struct Keys { QByteArray enc, auth, pwv; };
Keys deriveKeys(const QString &password, const QByteArray &salt, int keyLen)
{
    const QByteArray pw = password.toUtf8();
    QByteArray out(2 * keyLen + 2, 0);
    if (PKCS5_PBKDF2_HMAC(pw.constData(), int(pw.size()), u(salt), int(salt.size()), 1000, EVP_sha1(), int(out.size()), u(out)) != 1)
        throw std::runtime_error("PBKDF2-SHA1 failed");
    Keys k{out.left(keyLen), out.mid(keyLen, keyLen), out.right(2)};
    OPENSSL_cleanse(out.data(), size_t(out.size()));
    return k;
}

QByteArray hmacSha1_10(const QByteArray &key, const QByteArray &data)
{
    unsigned char mac[EVP_MAX_MD_SIZE];
    unsigned int n = 0;
    HMAC(EVP_sha1(), key.constData(), int(key.size()), u(data), size_t(data.size()), mac, &n);
    return QByteArray(reinterpret_cast<char *>(mac), 10);
}

bool ctEqual(const QByteArray &a, const QByteArray &b)
{
    if (a.size() != b.size()) return false;
    unsigned char d = 0;
    for (int i = 0; i < a.size(); ++i) d |= quint8(a[i]) ^ quint8(b[i]);
    return d == 0;
}

QByteArray aesExtra(int strength)
{
    QByteArray e;
    put16(e, 0x9901);
    put16(e, 7);
    put16(e, 2);                 // AE-2: CRC field is 0, integrity comes from the HMAC
    e.append("AE");
    e.append(char(strength));
    put16(e, 0);                 // real compression method: stored
    return e;
}

} // namespace

Result writeAesZip(const QString &path, const QList<Entry> &entries, const QString &password)
{
    try {
        if (entries.size() >= 65535) return Result::fail(QObject::tr("檔案數量過多"));
        const QDateTime now = QDateTime::currentDateTime();
        const quint16 dosTime = quint16((now.time().hour() << 11) | (now.time().minute() << 5) | (now.time().second() / 2));
        const quint16 dosDate = quint16(((qMax(1980, now.date().year()) - 1980) << 9) | (now.date().month() << 5) | now.date().day());
        const int strength = 3;
        const AesParams ap = paramsFor(strength);

        QByteArray body, central;
        for (const Entry &e : entries) {
            const QByteArray name = e.name.toUtf8();
            const QByteArray salt = randomBytes(ap.saltLen);
            const Keys k = deriveKeys(password, salt, ap.keyLen);
            const QByteArray enc = winzipCtr(k.enc, e.data);
            const QByteArray payload = salt + k.pwv + enc + hmacSha1_10(k.auth, enc);
            if (payload.size() > 0x7FFFFFFF || body.size() > 0x7FFFFFFF) return Result::fail(QObject::tr("壓縮包過大"));
            const QByteArray extra = aesExtra(strength);
            const quint32 offset = quint32(body.size());

            QByteArray lh;
            put32(lh, kLocalSig);
            put16(lh, 51);                              // version needed (5.1 = AES)
            put16(lh, kFlagEncrypted | kFlagUtf8);
            put16(lh, kMethodAes);
            put16(lh, dosTime);
            put16(lh, dosDate);
            put32(lh, 0);                               // CRC-32 (0 for AE-2)
            put32(lh, quint32(payload.size()));
            put32(lh, quint32(e.data.size()));
            put16(lh, quint16(name.size()));
            put16(lh, quint16(extra.size()));
            body += lh + name + extra + payload;

            QByteArray ch;
            put32(ch, kCentralSig);
            put16(ch, 0x033F);                          // made by: Unix, 6.3
            put16(ch, 51);
            put16(ch, kFlagEncrypted | kFlagUtf8);
            put16(ch, kMethodAes);
            put16(ch, dosTime);
            put16(ch, dosDate);
            put32(ch, 0);
            put32(ch, quint32(payload.size()));
            put32(ch, quint32(e.data.size()));
            put16(ch, quint16(name.size()));
            put16(ch, quint16(extra.size()));
            put16(ch, 0);                               // comment
            put16(ch, 0);                               // disk
            put16(ch, 0);                               // internal attrs
            put32(ch, 0x81A40000);                      // external attrs: -rw-r--r--
            put32(ch, offset);
            central += ch + name + extra;
        }
        QByteArray end;
        put32(end, kEndSig);
        put16(end, 0);
        put16(end, 0);
        put16(end, quint16(entries.size()));
        put16(end, quint16(entries.size()));
        put32(end, quint32(central.size()));
        put32(end, quint32(body.size()));
        put16(end, 0);

        QSaveFile f(path);
        if (!f.open(QIODevice::WriteOnly)) return Result::fail(f.errorString());
        f.write(body);
        f.write(central);
        f.write(end);
        if (!f.commit()) return Result::fail(f.errorString());
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

Result readAesZip(const QString &path, const QString &password, QList<Entry> *out)
{
    try {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return Result::fail(QObject::tr("無法開啟壓縮包：%1").arg(f.errorString()));
        if (f.size() > kMaxArchive) return Result::fail(QObject::tr("壓縮包過大"));
        const QByteArray z = f.readAll();
        const QString bad = QObject::tr("不是有效的備份壓縮包，或檔案已損毀");

        qint64 eocd = -1;
        for (qint64 i = z.size() - 22; i >= 0 && i >= z.size() - 22 - 65535; --i)
            if (get32(z, i) == kEndSig) { eocd = i; break; }
        if (eocd < 0) return Result::fail(bad);
        const int count = get16(z, eocd + 10);
        qint64 pos = get32(z, eocd + 16);
        QList<Entry> res;
        for (int n = 0; n < count; ++n) {
            if (pos + 46 > z.size() || get32(z, pos) != kCentralSig) return Result::fail(bad);
            const quint16 flags = get16(z, pos + 8), method = get16(z, pos + 10);
            const quint32 compSize = get32(z, pos + 20), rawSize = get32(z, pos + 24);
            const quint16 nameLen = get16(z, pos + 28), extraLen = get16(z, pos + 30), commentLen = get16(z, pos + 32);
            const quint32 lho = get32(z, pos + 42);
            if (pos + 46 + nameLen + extraLen + commentLen > z.size()) return Result::fail(bad);
            const QString name = QString::fromUtf8(z.mid(pos + 46, nameLen));
            const QByteArray extra = z.mid(pos + 46 + nameLen, extraLen);
            pos += 46 + nameLen + extraLen + commentLen;
            if (name.endsWith(QLatin1Char('/'))) continue;                                  // directory entry

            if (!(flags & kFlagEncrypted) || method != kMethodAes)
                return Result::fail(QObject::tr("壓縮包內的「%1」沒有使用 AES 加密，不是本程式匯出的檔案").arg(name));
            int strength = 0, realMethod = -1;
            for (int e = 0; e + 4 <= extra.size();) {
                const quint16 id = get16(extra, e), sz = get16(extra, e + 2);
                if (id == 0x9901 && sz >= 7 && e + 4 + 7 <= extra.size()) {
                    strength = quint8(extra[e + 4 + 4]);
                    realMethod = get16(extra, e + 4 + 5);
                }
                e += 4 + sz;
            }
            if (strength < 1 || strength > 3) return Result::fail(bad);
            if (realMethod != 0) return Result::fail(QObject::tr("壓縮包內的「%1」使用了壓縮；請勿用其他軟體重新壓縮，直接使用匯出的原始檔案").arg(name));
            const AesParams ap = paramsFor(strength);

            if (qint64(lho) + 30 > z.size() || get32(z, lho) != kLocalSig) return Result::fail(bad);
            const qint64 dataStart = qint64(lho) + 30 + get16(z, lho + 26) + get16(z, lho + 28);
            if (dataStart + qint64(compSize) > z.size() || qint64(compSize) < ap.saltLen + 2 + 10) return Result::fail(bad);
            const QByteArray salt = z.mid(dataStart, ap.saltLen);
            const QByteArray pwv = z.mid(dataStart + ap.saltLen, 2);
            const qint64 encLen = qint64(compSize) - ap.saltLen - 2 - 10;
            const QByteArray enc = z.mid(dataStart + ap.saltLen + 2, encLen);
            const QByteArray mac = z.mid(dataStart + ap.saltLen + 2 + encLen, 10);

            const Keys k = deriveKeys(password, salt, ap.keyLen);
            if (!ctEqual(k.pwv, pwv)) return Result::fail(wrongPasswordMarker());
            if (!ctEqual(hmacSha1_10(k.auth, enc), mac)) return Result::fail(QObject::tr("壓縮包內的「%1」驗證失敗，檔案可能已損毀或被竄改").arg(name));
            Entry e;
            e.name = name;
            e.data = winzipCtr(k.enc, enc);
            if (quint32(e.data.size()) != rawSize) return Result::fail(bad);
            res.append(e);
        }
        *out = res;
        return Result::success();
    } catch (const std::exception &e) {
        return Result::fail(QString::fromUtf8(e.what()));
    }
}

} // namespace tv::zip
