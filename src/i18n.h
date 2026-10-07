#pragma once
// Runtime i18n. Source strings are Traditional Chinese (zh_TW) in tr() calls; other languages are Qt Linguist .ts
// files (translations/TokenVault_<lang>.ts) embedded as resources and parsed at start-up - no lupdate/lrelease
// step, nothing to install in CI. Lookup is by source text (context is ignored), so one flat table per language.
#include <QHash>
#include <QString>
#include <QStringList>
#include <QTranslator>

namespace tv::i18n {

class TsTranslator : public QTranslator {
public:
    bool loadTs(const QString &path, QString *err = nullptr);          // file system path or ":/resource"
    bool isEmpty() const override { return m_map.isEmpty(); }
    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const override;
    int count() const { return int(m_map.size()); }
    int unfinished() const { return m_unfinished; }
private:
    QHash<QString, QString> m_map;
    int m_unfinished = 0;
};

QStringList supportedLanguages();                  // {"zh_TW", "en"}
QString preferredLanguage();                       // "auto" | "zh_TW" | "en"  ($TOKENVAULT_LANG overrides)
void setPreferredLanguage(const QString &lang);
QString effectiveLanguage();                       // resolves "auto" using the system locale
void install();                                    // call once, right after QApplication is created

} // namespace tv::i18n
