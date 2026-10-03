#include "icons.h"
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QObject>
#include <QtMath>
#include <functional>

static constexpr qreal kPi = 3.14159265358979323846;

namespace icons {

static const QColor kColors[] = {
    QColor("#4C8DF6"), QColor("#26A69A"), QColor("#F5A623"), QColor("#E5534B"),
    QColor("#9B6DFF"), QColor("#EC6FA8"), QColor("#6B7A8F"), QColor("#7CB342"),
};

int shapeCount() { return 8; }
int colorCount() { return int(sizeof(kColors) / sizeof(kColors[0])); }
QColor color(int idx) { return kColors[qBound(0, idx, colorCount() - 1)]; }

QString shapeName(int idx)
{
    static const char *n[] = {"圓形", "方形", "菱形", "三角形", "星形", "六邊形", "愛心", "盾牌"};
    return QObject::tr(n[qBound(0, idx, shapeCount() - 1)]);
}

QString makeSpec(int shape, int c) { return QStringLiteral("%1:%2").arg(shape).arg(c); }

void parseSpec(const QString &spec, int *shape, int *c)
{
    const QStringList p = spec.split(':');
    int s = p.value(0).toInt(), col = p.value(1).toInt();
    *shape = qBound(0, s, shapeCount() - 1);
    *c = qBound(0, col, colorCount() - 1);
}

static QPainterPath shapePath(int shape, qreal s)   // fits in [0,s]x[0,s] with margin
{
    QPainterPath p;
    const qreal m = s * 0.08, w = s - 2 * m;
    const QPointF c(s / 2, s / 2);
    switch (shape) {
    case 0: p.addEllipse(QRectF(m, m, w, w)); break;
    case 1: p.addRoundedRect(QRectF(m, m, w, w), s * 0.18, s * 0.18); break;
    case 2: p.moveTo(c.x(), m); p.lineTo(s - m, c.y()); p.lineTo(c.x(), s - m); p.lineTo(m, c.y()); p.closeSubpath(); break;
    case 3: p.moveTo(c.x(), m); p.lineTo(s - m, s - m * 1.4); p.lineTo(m, s - m * 1.4); p.closeSubpath(); break;
    case 4: {
        const qreal R = w / 2, r = R * 0.45;
        for (int i = 0; i < 10; ++i) {
            const qreal a = -kPi / 2 + i * kPi / 5, rad = (i % 2 == 0) ? R : r;
            const QPointF pt(c.x() + rad * qCos(a), c.y() + rad * qSin(a));
            i == 0 ? p.moveTo(pt) : p.lineTo(pt);
        }
        p.closeSubpath();
        break;
    }
    case 5: {
        const qreal R = w / 2;
        for (int i = 0; i < 6; ++i) {
            const qreal a = i * kPi / 3;
            const QPointF pt(c.x() + R * qCos(a), c.y() + R * qSin(a));
            i == 0 ? p.moveTo(pt) : p.lineTo(pt);
        }
        p.closeSubpath();
        break;
    }
    case 6:
        p.moveTo(c.x(), s - m);
        p.cubicTo(m * 0.2, s * 0.58, m, m * 1.2, s * 0.30, m * 1.6);
        p.cubicTo(s * 0.42, m * 1.6, c.x(), s * 0.26, c.x(), s * 0.30);
        p.cubicTo(c.x(), s * 0.26, s * 0.58, m * 1.6, s * 0.70, m * 1.6);
        p.cubicTo(s - m, m * 1.2, s - m * 0.2, s * 0.58, c.x(), s - m);
        break;
    default:
        p.moveTo(c.x(), m);
        p.lineTo(s - m, s * 0.22);
        p.lineTo(s - m, s * 0.52);
        p.quadTo(s - m, s * 0.82, c.x(), s - m);
        p.quadTo(m, s * 0.82, m, s * 0.52);
        p.lineTo(m, s * 0.22);
        p.closeSubpath();
        break;
    }
    return p;
}

static QPixmap render(int logical, const std::function<void(QPainter &, qreal)> &draw)
{
    const int dpr = 2;
    QPixmap pm(logical * dpr, logical * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter g(&pm);
    g.setRenderHint(QPainter::Antialiasing);
    draw(g, logical);
    return pm;
}

QIcon shapeIcon(int shape, int c, int sz)
{
    return QIcon(render(sz, [&](QPainter &g, qreal s) {
        g.setPen(Qt::NoPen);
        g.setBrush(color(c));
        g.drawPath(shapePath(shape, s));
    }));
}

QIcon groupIcon(const QString &spec, int sz)
{
    int s, c;
    parseSpec(spec, &s, &c);
    return shapeIcon(s, c, sz);
}

QIcon statusIcon(tv::TokenInfo::Status st, int sz)
{
    const bool ok = st == tv::TokenInfo::Valid;
    return QIcon(render(sz, [&](QPainter &g, qreal s) {
        g.setBrush(ok ? QColor("#2E9E4F") : QColor("#D93F3F"));
        g.setPen(Qt::NoPen);
        g.drawEllipse(QRectF(s * 0.04, s * 0.04, s * 0.92, s * 0.92));
        QPen pen(Qt::white, s * 0.13, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        g.setPen(pen);
        g.setBrush(Qt::NoBrush);
        if (ok) {
            QPainterPath p;
            p.moveTo(s * 0.27, s * 0.52);
            p.lineTo(s * 0.44, s * 0.68);
            p.lineTo(s * 0.74, s * 0.34);
            g.drawPath(p);
        } else {
            g.drawLine(QPointF(s * 0.32, s * 0.32), QPointF(s * 0.68, s * 0.68));
            g.drawLine(QPointF(s * 0.68, s * 0.32), QPointF(s * 0.32, s * 0.68));
        }
    }));
}

QIcon appIcon()
{
    QIcon ic;
    for (int sz : {16, 32, 48, 64, 128, 256}) {
        ic.addPixmap(render(sz, [](QPainter &g, qreal s) {
            g.setPen(Qt::NoPen);
            g.setBrush(QColor("#3B6FD8"));
            g.drawPath(shapePath(7, s));
            g.setBrush(Qt::white);
            g.drawEllipse(QRectF(s * 0.38, s * 0.28, s * 0.24, s * 0.24));
            g.drawRoundedRect(QRectF(s * 0.46, s * 0.46, s * 0.08, s * 0.26), s * 0.03, s * 0.03);
        }));
    }
    return ic;
}

QString remainingText(const tv::TokenInfo &t)
{
    if (!t.expires.isValid()) return {};
    const qint64 secs = QDateTime::currentDateTimeUtc().secsTo(t.expires);
    if (secs <= 0) return QObject::tr("已過期");
    if (secs < 86400) return QObject::tr("剩 %1 小時").arg(qMax<qint64>(1, secs / 3600));
    return QObject::tr("剩 %1 天").arg((secs + 86399) / 86400);
}

} // namespace icons
