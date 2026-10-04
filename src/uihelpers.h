#pragma once
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {

// Re-evaluate the stylesheet after a dynamic property changed.
void repolish(QWidget *w);
void setProp(QWidget *w, const char *name, const QVariant &v);

QLabel *label(const QString &text, const char *role = nullptr, bool wrap = false);
QPushButton *button(const QString &text, const char *prop = nullptr);      // prop: "primary" | "danger" | "link"

// Round accent badge with a glyph + title + subtitle, used at the top of dialogs.
QWidget *header(const QPixmap &pm, const QString &title, const QString &subtitle = QString());
QPixmap badge(const QString &glyph, int size = 44);                          // accent circle with a text glyph
QPixmap lockBadge(int size = 44);

// Right-aligned [取消] [確定] row. Returns the layout; fills ok/cancel.
QLayout *footer(QDialog *d, QPushButton **ok, QPushButton **cancel,
                const QString &okText = QString(), const QString &cancelText = QString());

QFrame *card(QWidget *inner = nullptr);

} // namespace ui
