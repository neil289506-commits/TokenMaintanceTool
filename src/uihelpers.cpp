#include "uihelpers.h"
#include "theme.h"
#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>

namespace ui {

void repolish(QWidget *w)
{
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}

void setProp(QWidget *w, const char *name, const QVariant &v)
{
    w->setProperty(name, v);
    repolish(w);
}

QLabel *label(const QString &text, const char *role, bool wrap)
{
    auto *l = new QLabel(text);
    if (role) l->setProperty("role", role);
    l->setWordWrap(wrap);
    return l;
}

QPushButton *button(const QString &text, const char *prop)
{
    auto *b = new QPushButton(text);
    if (prop) b->setProperty(prop, true);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QPixmap badge(const QString &glyph, int size)
{
    const qreal dpr = 2;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter g(&pm);
    g.setRenderHint(QPainter::Antialiasing);
    g.setPen(Qt::NoPen);
    g.setBrush(theme::c().accent);
    g.drawRoundedRect(QRectF(0, 0, size, size), size * 0.3, size * 0.3);
    g.setPen(theme::c().accentText);
    QFont f = g.font();
    f.setPixelSize(int(size * 0.5));
    f.setBold(true);
    g.setFont(f);
    g.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, glyph);
    return pm;
}

QPixmap lockBadge(int size)
{
    const qreal dpr = 2;
    QPixmap pm(int(size * dpr), int(size * dpr));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter g(&pm);
    g.setRenderHint(QPainter::Antialiasing);
    g.setPen(Qt::NoPen);
    g.setBrush(theme::c().accent);
    g.drawRoundedRect(QRectF(0, 0, size, size), size * 0.3, size * 0.3);
    const QColor w = theme::c().accentText;
    // padlock: shackle + body + keyhole
    QPen pen(w, size * 0.075, Qt::SolidLine, Qt::RoundCap);
    g.setPen(pen);
    g.setBrush(Qt::NoBrush);
    QPainterPath sh;
    sh.moveTo(size * 0.36, size * 0.46);
    sh.lineTo(size * 0.36, size * 0.35);
    sh.arcTo(QRectF(size * 0.36, size * 0.17, size * 0.28, size * 0.36), 180, -180);
    sh.lineTo(size * 0.64, size * 0.46);
    g.drawPath(sh);
    g.setPen(Qt::NoPen);
    g.setBrush(w);
    g.drawRoundedRect(QRectF(size * 0.27, size * 0.44, size * 0.46, size * 0.34), size * 0.07, size * 0.07);
    g.setBrush(theme::c().accent);
    g.drawEllipse(QRectF(size * 0.46, size * 0.54, size * 0.08, size * 0.08));
    g.drawRect(QRectF(size * 0.48, size * 0.58, size * 0.04, size * 0.1));
    return pm;
}

QWidget *header(const QPixmap &pm, const QString &title, const QString &subtitle)
{
    auto *w = new QWidget;
    auto *row = new QHBoxLayout(w);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(14);
    auto *ic = new QLabel;
    ic->setPixmap(pm);
    ic->setFixedSize(pm.deviceIndependentSize().toSize());
    row->addWidget(ic, 0, Qt::AlignTop);
    auto *col = new QVBoxLayout;
    col->setSpacing(2);
    col->addWidget(label(title, "title"));
    if (!subtitle.isEmpty()) col->addWidget(label(subtitle, "subtitle", true));
    row->addLayout(col, 1);
    return w;
}

QLayout *footer(QDialog *d, QPushButton **ok, QPushButton **cancel, const QString &okText, const QString &cancelText)
{
    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 6, 0, 0);
    row->addStretch(1);
    auto *c = button(cancelText.isEmpty() ? QObject::tr("取消") : cancelText);
    auto *o = button(okText.isEmpty() ? QObject::tr("確定") : okText, "primary");
    o->setDefault(true);
    row->addWidget(c);
    row->addWidget(o);
    QObject::connect(c, &QPushButton::clicked, d, &QDialog::reject);
    QObject::connect(o, &QPushButton::clicked, d, &QDialog::accept);
    if (ok) *ok = o;
    if (cancel) *cancel = c;
    return row;
}

QFrame *card(QWidget *inner)
{
    auto *f = new QFrame;
    f->setProperty("card", true);
    if (inner) {
        auto *l = new QVBoxLayout(f);
        l->setContentsMargins(14, 12, 14, 12);
        l->addWidget(inner);
    }
    return f;
}

} // namespace ui
