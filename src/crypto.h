#pragma once
// Thin OpenSSL wrappers. Everything secret lives in SecureBytes (zeroed on free).
#include <QByteArray>
#include <QString>
#include <openssl/evp.h>
#include <memory>

namespace tv {

// Byte buffer that is cleansed with OPENSSL_cleanse on destruction/reassignment.
class SecureBytes {
public:
    SecureBytes() = default;
    explicit SecureBytes(const QByteArray &d) : m_d(d) { m_d.detach(); }
    SecureBytes(const SecureBytes &o) : m_d(o.m_d) { m_d.detach(); }
    SecureBytes(SecureBytes &&o) noexcept : m_d(o.m_d) { m_d.detach(); o.wipe(); }
    SecureBytes &operator=(const SecureBytes &o) { if (this != &o) { wipe(); m_d = o.m_d; m_d.detach(); } return *this; }
    SecureBytes &operator=(SecureBytes &&o) noexcept { if (this != &o) { wipe(); m_d = o.m_d; m_d.detach(); o.wipe(); } return *this; }
    ~SecureBytes() { wipe(); }
    const QByteArray &data() const { return m_d; }
    QByteArray &data() { return m_d; }
    bool isEmpty() const { return m_d.isEmpty(); }
    int size() const { return int(m_d.size()); }
    void wipe();
private:
    QByteArray m_d;
};

struct PKeyDeleter { void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); } };
using PKey = std::unique_ptr<EVP_PKEY, PKeyDeleter>;

constexpr int kAesKeyLen = 32;
constexpr int kGcmIvLen = 12;
constexpr int kGcmTagLen = 16;
constexpr int kRsaBits = 4096;

QByteArray randomBytes(int n);                 // CSPRNG (RAND_bytes); throws on failure
QString randomId(int len = 6);                 // [A-Za-z0-9], unbiased
QByteArray sha256(const QByteArray &d);
QByteArray sha512(const QByteArray &d);
// Streaming SHA-512 of a file. Returns empty + sets err on failure.
QByteArray sha512File(const QString &path, QString *err);

// PBKDF2-HMAC-SHA512
SecureBytes pbkdf2Sha512(const QByteArray &material, const QByteArray &salt, int iterations, int outLen);

// AES-256-GCM. Output layout: iv(12) | ciphertext | tag(16)
QByteArray aesGcmSeal(const SecureBytes &key, const QByteArray &plain, const QByteArray &aad);
// Returns false on authentication failure (wrong key / tampered data).
bool aesGcmOpen(const SecureBytes &key, const QByteArray &blob, const QByteArray &aad, SecureBytes *plain);

// RSA-4096
PKey rsaGenerate(int bits = kRsaBits);
QByteArray pubToDer(EVP_PKEY *k);
PKey pubFromDer(const QByteArray &der);
SecureBytes privToDer(EVP_PKEY *k);
PKey privFromDer(const SecureBytes &der);
// RSA-OAEP, SHA-256 for both OAEP digest and MGF1
QByteArray rsaOaepEncrypt(EVP_PKEY *pub, const QByteArray &plain);
bool rsaOaepDecrypt(EVP_PKEY *priv, const QByteArray &cipher, SecureBytes *plain);

} // namespace tv
