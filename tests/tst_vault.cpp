#include "vault.h"
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

using namespace tv;

class VaultTest : public QObject {
    Q_OBJECT
private slots:
    void roundTripBoth()
    {
        QTemporaryDir td;
        QTemporaryFile kf;
        QVERIFY(kf.open());
        kf.write("some key file content");
        kf.flush();
        Credentials c{"pass-123", kf.fileName()};

        Vault v(td.path());
        QVERIFY(!v.exists());
        QVERIFY2(v.create(AuthBoth, c), "create");
        QVERIFY(v.exists());
        QCOMPARE(v.mode(), AuthBoth);

        QString gid;
        QVERIFY(v.createGroup("GitHub", "my gh tokens", "star:2", &gid));
        QCOMPARE(gid.size(), 6);
        const QDateTime exp = QDateTime::currentDateTimeUtc().addDays(7);
        QString tid;
        const QString secret = QString::fromUtf8("ghp_abc123_測試🔑");
        QVERIFY(v.addToken(gid, "CI", "for ci", exp, SecureBytes(secret.toUtf8()), &tid));
        QVERIFY(QFile::exists(td.path() + "/Token/" + gid + "/" + tid + ".tkn"));

        // file must not contain plaintext
        QFile f(v.tokenFile(gid, tid));
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray raw = f.readAll();
        QVERIFY(!raw.contains("ghp_abc123"));
        QVERIFY(!raw.contains("for ci"));
        f.close();

        // fresh session: lock, unlock, list
        v.lock();
        QVERIFY(!v.isUnlocked());
        QVERIFY(!v.unlock({"wrong", kf.fileName()}));
        QVERIFY(!v.unlock({"pass-123", ""}));
        QVERIFY(v.unlock(c));
        auto gs = v.groups();
        QCOMPARE(gs.size(), 1);
        QCOMPARE(gs[0].name, QString("GitHub"));
        auto ts = v.tokens(gid);
        QCOMPARE(ts.size(), 1);
        QCOMPARE(ts[0].name, QString("CI"));
        QCOMPARE(ts[0].status(), TokenInfo::Valid);
        QVERIFY(ts[0].daysLeft() >= 6);

        SecureBytes out;
        QVERIFY(!v.revealToken(gid, tid, {"nope", kf.fileName()}, &out));
        QVERIFY2(v.revealToken(gid, tid, c, &out), "reveal");
        QCOMPARE(QString::fromUtf8(out.data()), secret);

        // meta edit without touching secret
        TokenInfo t = ts[0];
        t.name = "CI2";
        t.revoked = true;
        QVERIFY(v.updateTokenMeta(t));
        QCOMPARE(v.tokens(gid)[0].name, QString("CI2"));
        QCOMPARE(v.tokens(gid)[0].status(), TokenInfo::Revoked);
        QVERIFY(v.revealToken(gid, tid, c, &out));
        QCOMPARE(QString::fromUtf8(out.data()), secret);

        // renew
        QVERIFY(v.replaceTokenSecret(gid, tid, SecureBytes("new-secret"), true, QDateTime::currentDateTimeUtc().addDays(30), true));
        QCOMPARE(v.tokens(gid)[0].status(), TokenInfo::Valid);
        QVERIFY(v.revealToken(gid, tid, c, &out));
        QCOMPARE(out.data(), QByteArray("new-secret"));

        // tamper => detected
        QFile tf(v.tokenFile(gid, tid));
        QVERIFY(tf.open(QIODevice::ReadWrite));
        QByteArray all = tf.readAll();
        int p = all.indexOf("\"secret\": \"") + 14;
        all[p] = all[p] == 'A' ? 'B' : 'A';
        tf.seek(0); tf.write(all); tf.close();
        QVERIFY(!v.revealToken(gid, tid, c, &out));

        // change credentials: old ones stop working, tokens untouched
        QVERIFY(v.replaceTokenSecret(gid, tid, SecureBytes("again"), false, {}, false));
        QVERIFY(v.changeCredentials(c, AuthPassphrase, {"new-pass", ""}));
        QCOMPARE(v.mode(), AuthPassphrase);
        v.lock();
        QVERIFY(!v.unlock(c));
        QVERIFY(v.unlock({"new-pass", ""}));
        QVERIFY(v.revealToken(gid, tid, {"new-pass", ""}, &out));
        QCOMPARE(out.data(), QByteArray("again"));

        QVERIFY(v.deleteToken(gid, tid));
        QCOMPARE(v.tokens(gid).size(), 0);
        QVERIFY(v.deleteGroup(gid));
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
