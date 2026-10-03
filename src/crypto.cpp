#include "crypto.h"
#include <QFile>
#include <QCryptographicHash>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <stdexcept>

namespace tv {

void SecureBytes::wipe()
{
    if (!m_d.isEmpty()) {
        m_d.detach();
        OPENSSL_cleanse(m_d.data(), size_t(m_d.size()));
    }
    m_d.clear();
}

namespace {
struct CtxDel { void operator()(EVP_CIPHER_CTX *c) const { EVP_CIPHER_CTX_free(c); } };
struct PCtxDel { void operator()(EVP_PKEY_CTX *c) const { EVP_PKEY_CTX_free(c); } };
using Ctx = std::unique_ptr<EVP_CIPHER_CTX, CtxDel>;
using PCtx = std::unique_ptr<EVP_PKEY_CTX, PCtxDel>;
inline const unsigned char *u(const QByteArray &b) { return reinterpret_cast<const unsigned char *>(b.constData()); }
inline unsigned char *u(QByteArray &b) { return reinterpret_cast<unsigned char *>(b.data()); }
}

QByteArray randomBytes(int n)
{
    QByteArray b(n, 0);
    if (RAND_bytes(u(b), n) != 1)
        throw std::runtime_error("RAND_bytes failed");
    return b;
}

QString randomId(int len)
{
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    QString out;
    while (out.size() < len) {
        QByteArray r = randomBytes(len * 2);
        for (unsigned char c : r) {
            if (c >= 248) continue;          // 248 = 62*4 -> rejection sampling, no modulo bias
            out.append(QLatin1Char(alphabet[c % 62]));
            if (out.size() == len) break;
        }
    }
    return out;
}

QByteArray sha256(const QByteArray &d) { return QCryptographicHash::hash(d, QCryptographicHash::Sha256); }
QByteArray sha512(const QByteArray &d) { return QCryptographicHash::hash(d, QCryptographicHash::Sha512); }

QByteArray sha512File(const QString &path, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = f.errorString();
        return {};
    }
    QCryptographicHash h(QCryptographicHash::Sha512);
    if (!h.addData(&f)) {
        if (err) *err = f.errorString();
        return {};
    }
    return h.result();
}

SecureBytes pbkdf2Sha512(const QByteArray &material, const QByteArray &salt, int iterations, int outLen)
{
    QByteArray out(outLen, 0);
    if (PKCS5_PBKDF2_HMAC(material.constData(), int(material.size()),
                          u(salt), int(salt.size()),
                          iterations, EVP_sha512(), outLen, u(out)) != 1)
        throw std::runtime_error("PBKDF2 failed");
    SecureBytes s(out);
    OPENSSL_cleanse(out.data(), size_t(out.size()));
    return s;
}

QByteArray aesGcmSeal(const SecureBytes &key, const QByteArray &plain, const QByteArray &aad)
{
    if (key.size() != kAesKeyLen) throw std::runtime_error("bad AES key length");
    QByteArray iv = randomBytes(kGcmIvLen);
    Ctx c(EVP_CIPHER_CTX_new());
    QByteArray ct(int(plain.size()) + 16, 0);
    int len = 0, total = 0;
    QByteArray tag(kGcmTagLen, 0);
    if (!c || EVP_EncryptInit_ex(c.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1
        || EVP_CIPHER_CTX_ctrl(c.get(), EVP_CTRL_GCM_SET_IVLEN, kGcmIvLen, nullptr) != 1
        || EVP_EncryptInit_ex(c.get(), nullptr, nullptr, u(key.data()), u(iv)) != 1)
        throw std::runtime_error("AES init failed");
    if (!aad.isEmpty() && EVP_EncryptUpdate(c.get(), nullptr, &len, u(aad), int(aad.size())) != 1)
        throw std::runtime_error("AES aad failed");
    if (EVP_EncryptUpdate(c.get(), u(ct), &len, u(plain), int(plain.size())) != 1)
        throw std::runtime_error("AES update failed");
    total = len;
    if (EVP_EncryptFinal_ex(c.get(), u(ct) + total, &len) != 1)
        throw std::runtime_error("AES final failed");
    total += len;
    ct.resize(total);
    if (EVP_CIPHER_CTX_ctrl(c.get(), EVP_CTRL_GCM_GET_TAG, kGcmTagLen, u(tag)) != 1)
        throw std::runtime_error("AES tag failed");
    return iv + ct + tag;
}

bool aesGcmOpen(const SecureBytes &key, const QByteArray &blob, const QByteArray &aad, SecureBytes *plain)
{
    if (key.size() != kAesKeyLen || blob.size() < kGcmIvLen + kGcmTagLen) return false;
    const int ctLen = int(blob.size()) - kGcmIvLen - kGcmTagLen;
    QByteArray iv = blob.left(kGcmIvLen);
    QByteArray tag = blob.right(kGcmTagLen);
    QByteArray ct = blob.mid(kGcmIvLen, ctLen);
    QByteArray out(ctLen + 16, 0);
    Ctx c(EVP_CIPHER_CTX_new());
    int len = 0, total = 0;
    if (!c || EVP_DecryptInit_ex(c.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1
        || EVP_CIPHER_CTX_ctrl(c.get(), EVP_CTRL_GCM_SET_IVLEN, kGcmIvLen, nullptr) != 1
        || EVP_DecryptInit_ex(c.get(), nullptr, nullptr, u(key.data()), u(iv)) != 1)
        return false;
    if (!aad.isEmpty() && EVP_DecryptUpdate(c.get(), nullptr, &len, u(aad), int(aad.size())) != 1)
        return false;
    if (EVP_DecryptUpdate(c.get(), u(out), &len, u(ct), ctLen) != 1) return false;
    total = len;
    if (EVP_CIPHER_CTX_ctrl(c.get(), EVP_CTRL_GCM_SET_TAG, kGcmTagLen, u(tag)) != 1) return false;
    if (EVP_DecryptFinal_ex(c.get(), u(out) + total, &len) <= 0) {
        OPENSSL_cleanse(out.data(), size_t(out.size()));
        return false;
    }
    total += len;
    out.resize(total);
    *plain = SecureBytes(out);
    OPENSSL_cleanse(out.data(), size_t(out.size()));
    return true;
}

PKey rsaGenerate(int bits)
{
    PCtx ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
    EVP_PKEY *raw = nullptr;
    if (!ctx || EVP_PKEY_keygen_init(ctx.get()) <= 0
        || EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), bits) <= 0
        || EVP_PKEY_keygen(ctx.get(), &raw) <= 0)
        throw std::runtime_error("RSA keygen failed");
    return PKey(raw);
}

QByteArray pubToDer(EVP_PKEY *k)
{
    int n = i2d_PUBKEY(k, nullptr);
    if (n <= 0) throw std::runtime_error("i2d_PUBKEY failed");
    QByteArray b(n, 0);
    unsigned char *p = u(b);
    i2d_PUBKEY(k, &p);
    return b;
}

PKey pubFromDer(const QByteArray &der)
{
    const unsigned char *p = u(der);
    return PKey(d2i_PUBKEY(nullptr, &p, long(der.size())));
}

SecureBytes privToDer(EVP_PKEY *k)
{
    int n = i2d_PrivateKey(k, nullptr);
    if (n <= 0) throw std::runtime_error("i2d_PrivateKey failed");
    QByteArray b(n, 0);
    unsigned char *p = u(b);
    i2d_PrivateKey(k, &p);
    SecureBytes s(b);
    OPENSSL_cleanse(b.data(), size_t(b.size()));
    return s;
}

PKey privFromDer(const SecureBytes &der)
{
    const unsigned char *p = u(der.data());
    return PKey(d2i_AutoPrivateKey(nullptr, &p, long(der.size())));
}

QByteArray rsaOaepEncrypt(EVP_PKEY *pub, const QByteArray &plain)
{
    PCtx c(EVP_PKEY_CTX_new(pub, nullptr));
    if (!c || EVP_PKEY_encrypt_init(c.get()) <= 0
        || EVP_PKEY_CTX_set_rsa_padding(c.get(), RSA_PKCS1_OAEP_PADDING) <= 0
        || EVP_PKEY_CTX_set_rsa_oaep_md(c.get(), EVP_sha256()) <= 0
        || EVP_PKEY_CTX_set_rsa_mgf1_md(c.get(), EVP_sha256()) <= 0)
        throw std::runtime_error("RSA enc init failed");
    size_t outLen = 0;
    if (EVP_PKEY_encrypt(c.get(), nullptr, &outLen, u(plain), size_t(plain.size())) <= 0)
        throw std::runtime_error("RSA enc size failed");
    QByteArray out(int(outLen), 0);
    if (EVP_PKEY_encrypt(c.get(), u(out), &outLen, u(plain), size_t(plain.size())) <= 0)
        throw std::runtime_error("RSA enc failed");
    out.resize(int(outLen));
    return out;
}

bool rsaOaepDecrypt(EVP_PKEY *priv, const QByteArray &cipher, SecureBytes *plain)
{
    PCtx c(EVP_PKEY_CTX_new(priv, nullptr));
    if (!c || EVP_PKEY_decrypt_init(c.get()) <= 0
        || EVP_PKEY_CTX_set_rsa_padding(c.get(), RSA_PKCS1_OAEP_PADDING) <= 0
        || EVP_PKEY_CTX_set_rsa_oaep_md(c.get(), EVP_sha256()) <= 0
        || EVP_PKEY_CTX_set_rsa_mgf1_md(c.get(), EVP_sha256()) <= 0)
        return false;
    size_t outLen = 0;
    if (EVP_PKEY_decrypt(c.get(), nullptr, &outLen, u(cipher), size_t(cipher.size())) <= 0) return false;
    QByteArray out(int(outLen), 0);
    if (EVP_PKEY_decrypt(c.get(), u(out), &outLen, u(cipher), size_t(cipher.size())) <= 0) {
        OPENSSL_cleanse(out.data(), size_t(out.size()));
        return false;
    }
    out.resize(int(outLen));
    *plain = SecureBytes(out);
    OPENSSL_cleanse(out.data(), size_t(out.size()));
    return true;
}

} // namespace tv
