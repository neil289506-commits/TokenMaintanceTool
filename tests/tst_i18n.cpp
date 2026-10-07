#include "i18n.h"
#include <QtTest>

using namespace tv::i18n;

class I18nTest : public QObject {
    Q_OBJECT
private slots:
    void englishTable()
    {
        TsTranslator t;
        QString err;
        QVERIFY2(t.loadTs(QStringLiteral(TV_SOURCE_DIR "/translations/TokenVault_en.ts"), &err), qPrintable(err));
        QVERIFY(!t.isEmpty());
        QCOMPARE(t.unfinished(), 0);                                         // every string translated
        QCOMPARE(t.translate("X", "取消"), QString("Cancel"));
        QCOMPARE(t.translate("X", "刪除全部"), QString("Delete everything"));   // also the typed confirmation phrase
        QVERIFY(t.translate("X", "這句話不存在").isNull());                     // unknown => null => caller falls back
        // placeholders survive translation
        QCOMPARE(t.translate("X", "請等待 %1 秒").arg(5), QString("Wait 5 s"));
        QVERIFY(t.translate("X", "• 這兩組金鑰只會顯示這一次，之後無法再查看或找回。\n• 請分開保存，不要和壓縮包放在一起。\n"
                                 "• 壓縮包內含目前的密碼與 TOTP 金鑰（皆已加密）。同時取得壓縮包與兩組金鑰的人，等於取得全部資料。\n"
                                 "• 壓縮包的檔案清單會以明文顯示群組與 Token 的名稱（ZIP 格式的限制），內容則已加密。").contains("only once"));
    }

    void placeholdersMatch()           // every translation uses exactly the same %N placeholders as its source
    {
        QFile f(QStringLiteral(TV_SOURCE_DIR "/translations/TokenVault_en.ts"));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QXmlStreamReader x(&f);
        QString src;
        int checked = 0;
        auto holders = [](const QString &s) {
            QStringList l;
            QRegularExpressionMatchIterator it = QRegularExpression("%\\d").globalMatch(s);
            while (it.hasNext()) l << it.next().captured();
            l.sort();
            return l;
        };
        while (!x.atEnd()) {
            x.readNext();
            if (!x.isStartElement()) continue;
            if (x.name() == QLatin1String("source")) src = x.readElementText();
            else if (x.name() == QLatin1String("translation")) {
                const QString tr = x.readElementText();
                QVERIFY2(holders(src) == holders(tr), qPrintable("placeholder mismatch: " + src));
                ++checked;
            }
        }
        QVERIFY(checked > 200);
    }

    void zhTableHasQtButtons()
    {
        TsTranslator t;
        QVERIFY(t.loadTs(QStringLiteral(TV_SOURCE_DIR "/translations/TokenVault_zh_TW.ts")));
        QCOMPARE(t.translate("QPlatformTheme", "OK"), QString::fromUtf8("確定"));
        QCOMPARE(t.translate("QPlatformTheme", "Cancel"), QString::fromUtf8("取消"));
    }

    void languageResolution()
    {
        qputenv("TOKENVAULT_LANG", "en");
        QCOMPARE(effectiveLanguage(), QString("en"));
        qputenv("TOKENVAULT_LANG", "zh_TW");
        QCOMPARE(effectiveLanguage(), QString("zh_TW"));
        qputenv("TOKENVAULT_LANG", "xx");                                      // unsupported => ignored
        QVERIFY(supportedLanguages().contains(effectiveLanguage()));
        qunsetenv("TOKENVAULT_LANG");
    }
};

QTEST_APPLESS_MAIN(I18nTest)
#include "tst_i18n.moc"
