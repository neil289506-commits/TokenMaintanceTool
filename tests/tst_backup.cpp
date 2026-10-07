#include "aeszip.h"
#include "backup.h"
#include "totp.h"
#include "vault.h"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

using namespace tv;

static QByteArray pattern(int n)
{
    QByteArray b(n, 0);
    for (int i = 0; i < n; ++i) b[i] = char((i * 7 + 3) % 251);
    return b;
}

class BackupTest : public QObject {
    Q_OBJECT
private slots:
    void zipRoundTrip()
    {
        QTemporaryDir td;
        const QString path = td.path() + "/t.zip";
        QList<zip::Entry> in;
        for (int n : {0, 1, 15, 16, 17, 31, 32, 33, 1000, 100000}) in.append({QString("dir/file%1.bin").arg(n), pattern(n)});
        in.append({QString::fromUtf8("群組/子目錄/檔案 é.ini"), "unicode name"});
        QVERIFY(zip::writeAesZip(path, in, "correct horse"));

        QList<zip::Entry> out;
        QVERIFY(zip::readAesZip(path, "correct horse", &out));
        QCOMPARE(out.size(), in.size());
        for (int i = 0; i < in.size(); ++i) {
            QCOMPARE(out[i].name, in[i].name);
            QCOMPARE(out[i].data, in[i].data);
        }
        const Result bad = zip::readAesZip(path, "wrong", &out);
        QVERIFY(!bad);
        QCOMPARE(bad.error, zip::wrongPasswordMarker());

        // flip one ciphertext byte: HMAC must catch it
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadWrite));
        QByteArray all = f.readAll();
        const int mid = int(all.size() / 2);                        // inside the 100 000-byte entry's ciphertext
        all[mid] = char(all[mid] ^ 0x01);
        f.seek(0);
        f.write(all);
        f.close();
        QVERIFY(!zip::readAesZip(path, "correct horse", &out));
        // truncated / garbage
        QVERIFY(!zip::readAesZip(path + ".nope", "x", &out));
    }

    void foreignZipFromPyzipper()      // run by tests/interop_zip.py with TV_TEST_ZIP_IN / TV_TEST_ZIP_PW
    {
        const QString in = qEnvironmentVariable("TV_TEST_ZIP_IN");
        if (in.isEmpty()) QSKIP("TV_TEST_ZIP_IN not set");
        QList<zip::Entry> out;
        QVERIFY2(zip::readAesZip(in, qEnvironmentVariable("TV_TEST_ZIP_PW"), &out), "read foreign zip");
        QMap<QString, QByteArray> m;
        for (const auto &e : out) m[e.name] = e.data;
        QCOMPARE(m.value("a.txt"), QByteArray("hello from pyzipper"));
        QCOMPARE(m.value(QString::fromUtf8("中文/子資料夾/檔案.ini")), QByteArray("[x]\na=1\n"));
        QCOMPARE(m.value("big.bin"), pattern(70000));
    }

    void sanitize()
    {
        QCOMPARE(sanitizeArchiveName("a/b\\c:d*e?f\"g<h>i|j"), QString("a_b_c_d_e_f_g_h_i_j"));
        QCOMPARE(sanitizeArchiveName(".."), QString("_"));
        QCOMPARE(sanitizeArchiveName("  trailing dots.. "), QString("trailing dots"));
        QCOMPARE(sanitizeArchiveName("CON"), QString("_CON"));
        QCOMPARE(sanitizeArchiveName("nul.txt"), QString("_nul.txt"));
        QCOMPARE(sanitizeArchiveName(""), QString("_"));
        QCOMPARE(sanitizeArchiveName(QString(200, 'x')).size(), 80);
        QCOMPARE(sanitizeArchiveName(QString::fromUtf8("群組一")), QString::fromUtf8("群組一"));
        QCOMPARE(normalizeBackupKey(" ab-cd ef\n"), QString("abcdef"));
        const QString k = randomId(48);
        QCOMPARE(normalizeBackupKey(formatBackupKey(k)), k);
    }

    void fullBackupAndRestore()
    {
        QTemporaryDir src, dst;
        QTemporaryFile kf;
        QVERIFY(kf.open());
        kf.write("key file bytes");
        kf.flush();
        const QByteArray secret = totp::newSecret();
        auto cred = [&] { return Credentials{"pass-w0rd", kf.fileName(), totp::code(secret, QDateTime::currentSecsSinceEpoch()), {}}; };
        const AuthMode mode = AuthPassphrase | AuthKeyfile | AuthTotp;

        Vault v(src.path());
        QVERIFY(v.create(mode, cred(), secret));
        const QByteArray png = QByteArray("\x89PNG\r\n\x1a\n", 8) + pattern(5000);
        QString g1, g2, g3, t;
        QVERIFY(v.createGroup(QString::fromUtf8("GitHub / CI"), "note\nwith newline", "4:2", &g1, png));
        QVERIFY(v.createGroup(QString::fromUtf8("GitHub / CI"), "same name twice", "1:1", &g2));      // duplicate group name
        QVERIFY(v.createGroup("README.txt", "", "0:0", &g3));                                          // clashes with a reserved file name
        const QDateTime exp = QDateTime::currentDateTimeUtc().addDays(10);
        QVERIFY(v.addToken(g1, "deploy", "multi\nline\\note", exp, SecureBytes(QString::fromUtf8("ghp_測試🔑").toUtf8()), &t));
        QVERIFY(v.addToken(g1, "deploy", "same token name", {}, SecureBytes("second"), &t));          // duplicate token name
        QVERIFY(v.addToken(g1, "group", "named like the group file", {}, SecureBytes("third"), &t));
        QVERIFY(v.addToken(g2, "con", "", {}, SecureBytes(QByteArray(3000, 'x')), &t));                 // long secret, reserved name
        QVERIFY(v.addToken(g3, QString::fromUtf8("日本語/名前"), "", {}, SecureBytes("jp"), &t));
        for (TokenInfo ti : v.tokens(g1)) {                         // file order is random, so pick by note
            if (ti.note != "multi\nline\\note") continue;
            ti.revoked = true;
            QVERIFY(v.updateTokenMeta(ti));
        }

        BackupData data;
        QVERIFY2(v.collectBackup(cred(), &data), "collect");
        QCOMPARE(data.groups.size(), 3);
        QVERIFY(!v.collectBackup({"wrong", kf.fileName(), cred().totpCode, {}}, &data));       // verification gate

        QElapsedTimer tm;
        tm.start();
        PKey rsa = generateBackupRsa();
        qInfo() << "RSA-8192 keygen ms:" << tm.elapsed();
        const QString zipPath = dst.path() + "/backup.zip";
        BackupKeys keys;
        QVERIFY2(writeBackup(data, rsa.get(), zipPath, &keys), "write");
        QCOMPARE(keys.zipKey.size(), 48);
        QCOMPARE(keys.rsaKey.size(), 48);
        QVERIFY(keys.zipKey != keys.rsaKey);
        for (QChar c : keys.zipKey + keys.rsaKey) QVERIFY(c.isLetterOrNumber() && c.unicode() < 128);
        const QString keep = qEnvironmentVariable("TV_TEST_ZIP_OUT");
        if (!keep.isEmpty()) {
            QFile::remove(keep);
            QVERIFY(QFile::copy(zipPath, keep));
            QFile kfile(keep + ".keys");
            QVERIFY(kfile.open(QIODevice::WriteOnly));
            kfile.write((keys.zipKey + "\n" + keys.rsaKey + "\n").toUtf8());
        }

        // plaintext must not be recoverable without the keys
        QFile zf(zipPath);
        QVERIFY(zf.open(QIODevice::ReadOnly));
        const QByteArray raw = zf.readAll();
        zf.close();
        QVERIFY(!raw.contains("pass-w0rd"));
        QVERIFY(!raw.contains("ghp_"));
        QVERIFY(!raw.contains("same token name"));                   // notes live inside the encrypted .ini files
        QVERIFY(!raw.contains("note\nwith newline"));
        // Known limitation of the ZIP format: entry NAMES (group / token names) are not encrypted.
        QVERIFY(raw.contains("deploy.ini"));

        BackupData back;
        QVERIFY(!readBackup(zipPath, "x", keys.rsaKey, &back));
        const Result wrongZip = readBackup(zipPath, randomId(48), keys.rsaKey, &back);
        QVERIFY(!wrongZip);
        qInfo() << "wrong zip key ->" << wrongZip.error;
        const Result wrongRsa = readBackup(zipPath, keys.zipKey, randomId(48), &back);
        QVERIFY(!wrongRsa);
        qInfo() << "wrong rsa key ->" << wrongRsa.error;
        QVERIFY2(readBackup(zipPath, formatBackupKey(keys.zipKey), " " + keys.rsaKey + " ", &back), "read (formatted keys)");

        QCOMPARE(int(back.mode), int(mode));
        QCOMPARE(back.passphrase, QString("pass-w0rd"));
        QCOMPARE(back.keyfileHash, sha512File(kf.fileName(), nullptr));
        QCOMPARE(back.totpSecret.data(), secret);
        QCOMPARE(back.groups.size(), 3);

        // restore into a brand-new location; the ORIGINAL passphrase / key file / authenticator still work
        Vault v2(dst.path() + "/restored");
        QVERIFY2(v2.importBackup(back), "import");
        QVERIFY(v2.isUnlocked());
        v2.lock();
        QVERIFY(!v2.unlock({"wrong", kf.fileName(), cred().totpCode, {}}));
        QVERIFY2(v2.unlock(cred()), "unlock restored with original credentials");
        const auto gs2 = v2.groups();
        QCOMPARE(gs2.size(), 3);
        int tokensTotal = 0;
        QSet<QString> secrets;
        for (const GroupInfo &g : gs2) {
            if (g.name == "README.txt") QCOMPARE(g.icon, QString("0:0"));
            if (g.note.startsWith("note")) {
                QCOMPARE(g.note, QString("note\nwith newline"));
                QCOMPARE(g.image, png);
            }
            for (const TokenInfo &ti2 : v2.tokens(g.id)) {
                ++tokensTotal;
                SecureBytes out;
                QVERIFY(v2.revealToken(g.id, ti2.id, cred(), &out));
                secrets.insert(QString::fromUtf8(out.data()));
                if (ti2.note == "multi\nline\\note") {
                    QVERIFY(ti2.revoked);                              // state preserved
                    QCOMPARE(ti2.expires.toSecsSinceEpoch(), exp.toSecsSinceEpoch());
                    QCOMPARE(QString::fromUtf8(out.data()), QString::fromUtf8("ghp_測試🔑"));
                }
            }
        }
        QCOMPARE(tokensTotal, 5);
        QVERIFY(secrets.contains("second") && secrets.contains("third") && secrets.contains("jp"));
        QVERIFY(secrets.contains(QString(3000, 'x')));

        // importing over an existing vault wipes it first
        Vault v3(src.path());
        QVERIFY2(v3.importBackup(back), "import over existing");
        QVERIFY(v3.unlock(cred()));
        QCOMPARE(v3.groups().size(), 3);
    }
};

QTEST_APPLESS_MAIN(BackupTest)
#include "tst_backup.moc"
