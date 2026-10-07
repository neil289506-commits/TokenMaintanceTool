#pragma once
#include <QColor>
#include <QString>

class QApplication;

// Fusion style + custom palette + stylesheet. Follows the OS light/dark setting (Qt >= 6.5) and
// re-applies itself when that changes.
namespace theme {

void install(QApplication &app);
bool isDark();

struct Colors {
    QColor window, card, cardHover, cardSelected, border, text, muted, accent, accentText, ok, warn, bad;
};
const Colors &c();

} // namespace theme
