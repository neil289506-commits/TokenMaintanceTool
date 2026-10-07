#include "theme.h"
#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleFactory>
#include <QStyleHints>

namespace theme {

static Colors g_colors;
static bool g_dark = false;

const Colors &c() { return g_colors; }
bool isDark() { return g_dark; }

static bool detectDark()
{
    const QString forced = qEnvironmentVariable("TOKENVAULT_THEME").toLower();     // "dark" | "light" overrides the OS
    if (forced == QLatin1String("dark")) return true;
    if (forced == QLatin1String("light")) return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const Qt::ColorScheme cs = QGuiApplication::styleHints()->colorScheme();
    if (cs != Qt::ColorScheme::Unknown) return cs == Qt::ColorScheme::Dark;
#endif
    return QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
}

static QString qss(const Colors &k)
{
    auto h = [](const QColor &c) { return c.name(QColor::HexRgb); };
    QString s;
    s += QStringLiteral(R"(
* { font-size: 13px; }
QWidget { color: %text; }
QDialog, QMessageBox { background: %window; }
QLabel[role="title"]    { font-size: 20px; font-weight: 700; }
QLabel[role="subtitle"] { color: %muted; }
QLabel[role="muted"]    { color: %muted; font-size: 12px; }
QLabel[role="error"]    { color: %bad; }
QLabel[role="warn"]     { color: %warn; }

QLineEdit, QPlainTextEdit, QComboBox, QDateTimeEdit, QSpinBox {
    background: %card; border: 1px solid %border; border-radius: 8px; padding: 7px 10px;
    selection-background-color: %accent; selection-color: %accentText;
}
QPlainTextEdit { padding: 6px 8px; }
QLineEdit:focus, QPlainTextEdit:focus, QComboBox:focus, QDateTimeEdit:focus { border: 1px solid %accent; }
QLineEdit:read-only { background: transparent; }
QLineEdit[otp="true"] { font-size: 24px; font-weight: 600; letter-spacing: 8px; padding: 8px 10px; }
QComboBox::drop-down, QDateTimeEdit::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView { background: %card; border: 1px solid %border; selection-background-color: %cardSelected; selection-color: %text; outline: none; }

QPushButton {
    background: %card; border: 1px solid %border; border-radius: 8px; padding: 7px 16px; min-height: 18px;
}
QPushButton:hover { background: %cardHover; }
QPushButton:pressed { background: %cardSelected; }
QPushButton:disabled { color: %muted; background: transparent; }
QPushButton[primary="true"] { background: %accent; color: %accentText; border: 1px solid %accent; font-weight: 600; }
QPushButton[primary="true"]:hover { background: %accentHover; }
QPushButton[primary="true"]:disabled { background: %border; border-color: %border; color: %muted; }
QPushButton[danger="true"] { color: %bad; }
QPushButton[danger="true"]:hover { background: %badSoft; }
QPushButton[link="true"] { background: transparent; border: none; color: %accent; padding: 2px 4px; }
QPushButton[link="true"]:hover { text-decoration: underline; background: transparent; }

QCheckBox { spacing: 8px; }

QFrame[card="true"] { background: %card; border: 1px solid %border; border-radius: 12px; }
QFrame[optionCard="true"] { background: %card; border: 1px solid %border; border-radius: 12px; }
QFrame[optionCard="true"][on="true"] { border: 1px solid %accent; background: %cardHover; }

QListView, QListWidget { background: transparent; border: none; outline: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %border; border-radius: 4px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background: %muted; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { height: 0; }

QMenu { background: %card; border: 1px solid %border; border-radius: 10px; padding: 6px; }
QMenu::item { padding: 8px 22px; border-radius: 6px; }
QMenu::item:selected { background: %cardSelected; }
QMenu::separator { height: 1px; background: %border; margin: 5px 8px; }
QMenuBar { background: transparent; }
QMenuBar::item { padding: 5px 10px; border-radius: 6px; background: transparent; }
QMenuBar::item:selected { background: %cardSelected; }
QToolTip { background: %card; color: %text; border: 1px solid %border; padding: 5px 8px; }
QSplitter::handle { background: %border; width: 1px; }
QProgressBar { background: %border; border: none; border-radius: 4px; height: 8px; }
QProgressBar::chunk { background: %accent; border-radius: 4px; }
QToolButton[fab="true"] { background: %accent; color: %accentText; border: none; border-radius: 28px; font-size: 30px; padding-bottom: 3px; }
QToolButton[fab="true"]:hover { background: %accentHover; }
QToolButton[fab="true"]:pressed { background: %accentPress; }
QToolButton[ghost="true"] { background: transparent; border: none; border-radius: 8px; padding: 6px 10px; }
QToolButton[ghost="true"]:hover { background: %cardSelected; }
QToolButton[swatch="true"] { border: 2px solid transparent; border-radius: 8px; padding: 3px; background: transparent; }
QToolButton[swatch="true"]:hover { background: %cardHover; }
QToolButton[swatch="true"]:checked { border: 2px solid %accent; background: %cardHover; }
)");
    const QColor accentHover = k.accent.lighter(112), accentPress = k.accent.darker(112);
    const QColor badSoft = QColor(k.bad.red(), k.bad.green(), k.bad.blue(), 36);
    s.replace("%accentHover", h(accentHover)).replace("%accentPress", h(accentPress)).replace("%accentText", h(k.accentText));
    s.replace("%badSoft", QStringLiteral("rgba(%1,%2,%3,0.14)").arg(k.bad.red()).arg(k.bad.green()).arg(k.bad.blue()));
    s.replace("%cardSelected", h(k.cardSelected)).replace("%cardHover", h(k.cardHover)).replace("%card", h(k.card));
    s.replace("%window", h(k.window)).replace("%border", h(k.border)).replace("%text", h(k.text)).replace("%muted", h(k.muted));
    s.replace("%accent", h(k.accent)).replace("%warn", h(k.warn)).replace("%bad", h(k.bad)).replace("%ok", h(k.ok));
    (void)badSoft;
    return s;
}

static void applyNow(QApplication &app)
{
    g_dark = detectDark();
    Colors k;
    if (g_dark) {
        k.window = "#16181d"; k.card = "#20232a"; k.cardHover = "#272b33"; k.cardSelected = "#2f3540";
        k.border = "#333845"; k.text = "#e6e8ee"; k.muted = "#8b93a5"; k.accent = "#5b8cff"; k.accentText = "#ffffff";
        k.ok = "#3fb872"; k.warn = "#e5a53a"; k.bad = "#f0605d";
    } else {
        k.window = "#f4f6fa"; k.card = "#ffffff"; k.cardHover = "#f0f4fc"; k.cardSelected = "#e3ebfb";
        k.border = "#dde2ec"; k.text = "#1b2030"; k.muted = "#6b7487"; k.accent = "#3b6fe0"; k.accentText = "#ffffff";
        k.ok = "#2a9d5c"; k.warn = "#c98300"; k.bad = "#d93f3f";
    }
    g_colors = k;

    QPalette p;
    p.setColor(QPalette::Window, k.window);
    p.setColor(QPalette::WindowText, k.text);
    p.setColor(QPalette::Base, k.card);
    p.setColor(QPalette::AlternateBase, k.cardHover);
    p.setColor(QPalette::Text, k.text);
    p.setColor(QPalette::Button, k.card);
    p.setColor(QPalette::ButtonText, k.text);
    p.setColor(QPalette::ToolTipBase, k.card);
    p.setColor(QPalette::ToolTipText, k.text);
    p.setColor(QPalette::Highlight, k.accent);
    p.setColor(QPalette::HighlightedText, k.accentText);
    p.setColor(QPalette::PlaceholderText, k.muted);
    p.setColor(QPalette::Disabled, QPalette::Text, k.muted);
    p.setColor(QPalette::Disabled, QPalette::WindowText, k.muted);
    app.setPalette(p);
    app.setStyleSheet(qss(k));
}

void install(QApplication &app)
{
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion"))) app.setStyle(fusion);
    QFont f = app.font();
    f.setFamilies({QStringLiteral("Segoe UI"), QStringLiteral("SF Pro Text"), QStringLiteral("Noto Sans CJK TC"),
                   QStringLiteral("Microsoft JhengHei UI"), QStringLiteral("PingFang TC"), QStringLiteral("Noto Sans")});
    app.setFont(f);
    applyNow(app);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, &app, [&app] { applyNow(app); });
#endif
}

} // namespace theme
