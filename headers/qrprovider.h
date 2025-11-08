#ifndef QRPROVIDER_H
#define QRPROVIDER_H

#include <QQuickImageProvider>

class QRImageProvider : public QQuickImageProvider {
public:
    QRImageProvider();
    QString currentData;
    void setData(const QString &data);
    QImage requestImage(const QString &id, QSize* size, const QSize &requestedSize) override;
private:
    QImage generateQRCode(const QString &data, const QSize &requestedSize);
};

#endif // QRPROVIDER_H
