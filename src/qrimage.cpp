#include "qrimage.h"
#include "qrcodegen.hpp"
#include <QColor>
#include <QPainter>

QImage makeQrImage(const QString &text, int scale, const QColor &fg, const QColor &bg)
{
    const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(text.toUtf8().constData(), qrcodegen::QrCode::Ecc::MEDIUM);
    const int quiet = 3, n = qr.getSize(), px = (n + 2 * quiet) * scale;
    QImage img(px, px, QImage::Format_RGB32);
    img.fill(bg);
    QPainter p(&img);
    p.setPen(Qt::NoPen);
    p.setBrush(fg);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            if (qr.getModule(x, y)) p.drawRect((x + quiet) * scale, (y + quiet) * scale, scale, scale);
    return img;
}
