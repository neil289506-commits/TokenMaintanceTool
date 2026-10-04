#include "totp.h"
#include "vault.h"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

using namespace tv;

class VaultTest : public QObject {
    Q_OBJECT
private slots:
    // ---- TOTP -------------------------------------------------------------------------------
    void totpRfcVectors()
    {
        // RFC 6238 appendix B (SHA-1, secret "12345678901234567890"), last 6 of the 8-digit values
        const QByteArray s("12345678901234567890");
        QCOMPARE(totp::code(s, 59), QString("287082"));
        QCOMPARE(totp::code(s, 1111111109), QString("081804"));
        QCOMPARE(totp::code(s, 1111111111), QString("050471"));
        QCOMPARE(totp::code(s, 1234567890), QString("005924"));
        QCOMPARE(totp::code(s, 2000000000), QString("279037"));
        QCOMPARE(totp::code(s, 59, 8), QString("94287082"));
    }

    void totpVerifyWindow()
    {
        const QByteArray s = totp::newSecret();
        const qint64 t = 1700000000;
        const QString now = totp::code(s, t);
        QVERIFY(totp::verify(s, now, t));
        QVERIFY(totp::verify(s, totp::code(s, t - 30), t));       // one step late
        QVERIFY(totp::verify(s, totp::code(s, t + 30), t));       // one step early
        QVERIFY(!totp::verify(s, totp::code(s, t + 90), t));      // outside window
        QVERIFY(!totp::verify(s, "12345", t));
        QVERIFY(!totp::verify(s, "abcdef", t));
        QVERIFY(totp::verify(s, now.left(3) + " " + now.mid(3), t));   // spaces tolerated
    }

    void base32()
    {
        QCOMPARE(totp::base32Encode("foobar"), QString("MZXW6YTBOI"));      // RFC 4648 vector (no padding)
        bool ok = false;
        QCOMPARE(totp::base32Decode("MZXW6YTBOI======", &ok), QByteArray("foobar"));
        QVERIFY(ok);
        QCOMPARE(totp::base32Decode("mzxw 6ytb oi", &ok), QByteArray("foobar"));
        totp::base32Decode("MZ1W", &ok);
        QVERIFY(!ok);
        for (int n = 0; n < 40; ++n) {
            const QByteArray r = randomBytes(n);
            QCOMPARE(totp::base32Decode(totp::base32Encode(r)), r);
        }
        const QString uri = totp::otpauthUri("TokenVault", "me", "12345678901234567890");
        QVERIFY(uri.startsWith("otpauth://totp/TokenVault%3Ame?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=TokenVault"));
    }

    // ---- mode rules ---------------------------------------------------------------------------
    void modeRules()
    {
        QVERIFY(validateMode(AuthPassphrase).isEmpty());
        QVERIFY(validateMode(AuthKeyfile).isEmpty());
        QVERIFY(validateMode(AuthPassphrase | AuthTotp).isEmpty());
        QVERIFY(validateMode(AuthKeyfile | AuthTotp).isEmpty());
        QVERIFY(validateMode(AuthPassphrase | AuthKeyfile | AuthTotp).isEmpty());
        QVERIFY(!validateMode(AuthPassphrase | AuthKeyfile).isEmpty());   // two-factor needs TOTP
        QVERIFY(!validateMode(AuthTotp).isEmpty());                       // TOTP alone is not a key
        QVERIFY(!validateMode(AuthMode(0)).isEmpty());
        QTemporaryDir td;
        Vault v(td.path());
        QVERIFY(!v.create(AuthPassphrase | AuthKeyfile, {"pw", "x", ""}));
        QVERIFY(!v.exists());
    }

    // ---- full round trip with every factor ------------------------------------------------------
    void roundTripAllFactors()
    {
        QTemporaryDir td;
        QTemporaryFile kf;
        QVERIFY(kf.open());
        kf.write("some key file content");
        kf.flush();
        const QByteArray secret = totp::newSecret();
        auto cred = [&](const QString &pw = "pass-123") {
            return Credentials{pw, kf.fileName(), totp::code(secret, QDateTime::currentSecsSinceEpoch())};
        };
        const AuthMode all = AuthPassphrase | AuthKeyfile | AuthTotp;

        Vault v(td.path());
        QVERIFY(!v.exists());
        QVERIFY2(v.create(all, cred(), secret), "create");
        QCOMPARE(int(v.mode()), int(all));

        QString gid;
        QVERIFY(v.createGroup("GitHub", "my gh tokens", "star:2", &gid));
        QCOMPARE(gid.size(), 6);
        const QDateTime exp = QDateTime::currentDateTimeUtc().addDays(7);
        QString tid;
        const QString token = QString::fromUtf8("ghp_abc123_測試🔑");
        QVERIFY(v.addToken(gid, "CI", "for ci", exp, SecureBytes(token.toUtf8()), &tid));

        QFile f(v.tokenFile(gid, tid));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray raw = f.readAll();
        QVERIFY(!raw.contains("ghp_abc123"));
        QVERIFY(!raw.contains("for ci"));
        f.close();
        QFile vf(v.vaultFile());
        QVERIFY(vf.open(QIODevice::ReadOnly));
        QVERIFY(!vf.readAll().contains(secret.toBase64()));              // TOTP secret is not stored in clear
        vf.close();

        v.lock();
        QVERIFY(!v.isUnlocked());
        QVERIFY(!v.unlock(cred("wrong")));
        QVERIFY(!v.unlock({"pass-123", "", totp::code(secret, QDateTime::currentSecsSinceEpoch())}));   // no key file
        Credentials badCode = cred();
        badCode.totpCode = totp::code(secret, QDateTime::currentSecsSinceEpoch() + 600);
        QVERIFY(!v.unlock(badCode));                                     // right password, wrong TOTP
        Credentials noCode = cred();
        noCode.totpCode.clear();
        QVERIFY(!v.unlock(noCode));
        QVERIFY(v.unlock(cred()));
        QCOMPARE(v.groups().size(), 1);
        auto ts = v.tokens(gid);
        QCOMPARE(ts.size(), 1);
        QCOMPARE(ts[0].status(), TokenInfo::Valid);

        SecureBytes out;
        QVERIFY(!v.revealToken(gid, tid, cred("nope"), &out));
        QVERIFY2(v.revealToken(gid, tid, cred(), &out), "reveal");
        QCOMPARE(QString::fromUtf8(out.data()), token);

        TokenInfo t = ts[0];
        t.name = "CI2";
        t.revoked = true;
        QVERIFY(v.updateTokenMeta(t));
        QCOMPARE(v.tokens(gid)[0].status(), TokenInfo::Revoked);
        QVERIFY(v.replaceTokenSecret(gid, tid, SecureBytes("new-secret"), true, QDateTime::currentDateTimeUtc().addDays(30), true));
        QCOMPARE(v.tokens(gid)[0].status(), TokenInfo::Valid);
        QVERIFY(v.revealToken(gid, tid, cred(), &out));
        QCOMPARE(out.data(), QByteArray("new-secret"));

        // tamper => detected
        QFile tf(v.tokenFile(gid, tid));
        QVERIFY(tf.open(QIODevice::ReadWrite));
        QByteArray all2 = tf.readAll();
        int p = all2.indexOf("\"secret\": \"") + 14;
        all2[p] = all2[p] == 'A' ? 'B' : 'A';
        tf.seek(0); tf.write(all2); tf.close();
        QVERIFY(!v.revealToken(gid, tid, cred(), &out));
        QVERIFY(v.replaceTokenSecret(gid, tid, SecureBytes("again"), false, {}, false));

        // change method: needs the CURRENT credentials; wrong ones are refused and change nothing
        QVERIFY(!v.changeCredentials(cred("bad"), AuthPassphrase, {"new-pass", "", ""}));
        QCOMPARE(int(v.mode()), int(all));
        QVERIFY(!v.changeCredentials(cred(), AuthPassphrase | AuthKeyfile, {"new-pass", kf.fileName(), ""}));   // rule: needs TOTP
        // keep the existing TOTP secret, drop the key file: password + TOTP
        QVERIFY2(v.changeCredentials(cred(), AuthPassphrase | AuthTotp, {"new-pass", "", ""}), "change");
        QCOMPARE(int(v.mode()), int(AuthPassphrase | AuthTotp));
        v.lock();
        QVERIFY(!v.unlock(cred()));
        const QString code = totp::code(secret, QDateTime::currentSecsSinceEpoch());
        QVERIFY2(v.unlock({"new-pass", "", code}), "unlock with kept TOTP secret");
        QVERIFY(v.revealToken(gid, tid, {"new-pass", "", code}, &out));
        QCOMPARE(out.data(), QByteArray("again"));

        // UI already verified the TOTP code moments ago: an empty/stale code is accepted, a wrong password is not
        QVERIFY(!v.changeCredentials({"wrong", "", ""}, AuthPassphrase | AuthTotp, {"new-pass", "", ""}, {}, true));
        QVERIFY(v.changeCredentials({"new-pass", "", ""}, AuthPassphrase | AuthTotp, {"new-pass", "", ""}, {}, true));

        // switch to password only (TOTP removed) and then a brand new TOTP secret
        QVERIFY(v.changeCredentials({"new-pass", "", code}, AuthPassphrase, {"third-pass", "", ""}));
        QCOMPARE(int(v.mode()), int(AuthPassphrase));
        const QByteArray secret2 = totp::newSecret();
        const QString c2 = totp::code(secret2, QDateTime::currentSecsSinceEpoch());
        QVERIFY(v.changeCredentials({"third-pass", "", ""}, AuthKeyfile | AuthTotp, {"", kf.fileName(), c2}, secret2));
        v.lock();
        QVERIFY(v.unlock({"", kf.fileName(), c2}));
        QVERIFY(v.revealToken(gid, tid, {"", kf.fileName(), c2}, &out));

        QVERIFY(v.deleteToken(gid, tid));
        QCOMPARE(v.tokens(gid).size(), 0);
        QVERIFY(v.deleteGroup(gid));
        QCOMPARE(v.groups().size(), 0);
    }

    // ---- "forgot password": everything is wiped, nothing else is touched --------------------------
    void forgotPasswordWipesEverything()
    {
        QTemporaryDir td;
        Vault v(td.path());
        QVERIFY(v.create(AuthPassphrase, {"pw", "", ""}));
        QString g, t;
        QVERIFY(v.createGroup("G", "", "0:0", &g));
        QVERIFY(v.addToken(g, "T", "", {}, SecureBytes("secret"), &t));
        QFile other(td.path() + "/unrelated.txt");
        QVERIFY(other.open(QIODevice::WriteOnly));
        other.write("keep me");
        other.close();

        QVERIFY2(v.wipeAll(), "wipe");
        QVERIFY(!v.exists());
        QVERIFY(!v.isUnlocked());
        QVERIFY(!QDir(td.path() + "/Token").exists());
        QVERIFY(QFile::exists(td.path() + "/unrelated.txt"));           // only our own data is removed
        // a fresh vault can be created afterwards, with different credentials
        QVERIFY(!v.create(AuthKeyfile | AuthTotp, {"", "", ""}, totp::newSecret()));   // key file path missing
        QVERIFY(v.create(AuthPassphrase, {"another", "", ""}));
        QCOMPARE(v.groups().size(), 0);
    }

    void expiry()
    {
        TokenInfo t;
        QCOMPARE(t.status(), TokenInfo::Valid);
        t.expires = QDateTime::currentDateTimeUtc().addSecs(-5);
        QCOMPARE(t.status(), TokenInfo::Expired);
        QVERIFY(t.daysLeft() < 0);
    }

    void randomIds()
    {
        for (int i = 0; i < 200; ++i) {
            QString s = randomId(6);
            QCOMPARE(s.size(), 6);
            for (QChar c : s) QVERIFY(c.isLetterOrNumber() && c.unicode() < 128);
        }
    }
};

QTEST_APPLESS_MAIN(VaultTest)
#include "tst_vault.moc"
