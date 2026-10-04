#pragma once
#include <QImage>
#include <QString>

// Renders `text` as a QR code (Nayuki qrcodegen, MIT, see third_party/qrcodegen). `scale` px per module.
QImage makeQrImage(const QString &text, int scale, const QColor &fg, const QColor &bg);
