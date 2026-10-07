#include "i18n.h"
#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QXmlStreamReader>

namespace tv::i18n {

bool TsTranslator::loadTs(const QString &path, QString *err)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = f.errorString();
        return false;
    }
    m_map.clear();
    m_unfinished = 0;
    QXmlStreamReader xml(&f);
    QString source;
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) continue;
        if (xml.name() == QLatin1String("message")) source.clear();
        else if (xml.name() == QLatin1String("source")) source = xml.readElementText(QXmlStreamReader::SkipChildElements);
        else if (xml.name() == QLatin1String("translation")) {
            const bool unfinished = xml.attributes().value(QLatin1String("type")) == QLatin1String("unfinished");
            const QString text = xml.readElementText(QXmlStreamReader::SkipChildElements);
            if (unfinished || text.isEmpty()) ++m_unfinished;
            else if (!source.isEmpty()) m_map.insert(source, text);
        }
    }
    if (xml.hasError()) {
        if (err) *err = xml.errorString();
        return false;
    }
    return true;
}

QString TsTranslator::translate(const char *, const char *sourceText, const char *, int) const
{
    if (!sourceText) return {};
    const auto it = m_map.constFind(QString::fromUtf8(sourceText));
    return it == m_map.constEnd() ? QString() : it.value();             // null => Qt falls back to the source text
}

QStringList supportedLanguages() { return {QStringLiteral("zh_TW"), QStringLiteral("en")}; }

QString preferredLanguage()
{
    const QString env = qEnvironmentVariable("TOKENVAULT_LANG");
    if (supportedLanguages().contains(env)) return env;
    const QString saved = QSettings().value(QStringLiteral("language"), QStringLiteral("auto")).toString();
    return (saved == QLatin1String("auto") || supportedLanguages().contains(saved)) ? saved : QStringLiteral("auto");
}

void setPreferredLanguage(const QString &lang) { QSettings().setValue(QStringLiteral("language"), lang); }

QString effectiveLanguage()
{
    const QString p = preferredLanguage();
    if (p != QLatin1String("auto")) return p;
    return QLocale::system().name().startsWith(QLatin1String("zh")) ? QStringLiteral("zh_TW") : QStringLiteral("en");
}

void install()
{
    // zh_TW is the source language, so its table only carries Qt's own standard button texts (OK, Cancel, ...).
    auto *t = new TsTranslator;
    if (t->loadTs(QStringLiteral(":/i18n/TokenVault_%1.ts").arg(effectiveLanguage()))) QCoreApplication::installTranslator(t);
    else delete t;
}

} // namespace tv::i18n
