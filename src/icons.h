#pragma once
#include "vault.h"
#include <QIcon>
#include <QColor>

namespace icons {

int shapeCount();
int colorCount();
QColor color(int idx);
QString shapeName(int idx);

QString makeSpec(int shape, int color);                 // "<shape>:<color>"
void parseSpec(const QString &spec, int *shape, int *color);

QIcon groupIcon(const QString &spec, int logicalSize = 32);
QIcon shapeIcon(int shape, int color, int logicalSize = 32);
QIcon statusIcon(tv::TokenInfo::Status st, int logicalSize = 20);   // green check / red cross
QIcon appIcon();

// "剩 N 小時" (<24h) or "剩 N 天" (rounded up). Empty if the token never expires.
QString remainingText(const tv::TokenInfo &t);

} // namespace icons
