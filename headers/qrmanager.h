#ifndef QRMANAGER_H
#define QRMANAGER_H

#include <QObject>
#include "qrprovider.h"

class QRManager : public QObject
{
    Q_OBJECT
public:
    explicit QRManager(QRImageProvider* provider, QObject *parent = nullptr);
    Q_INVOKABLE void updateQRCode(const QString &data);
signals:
    void qrCodeChanged();
private:
    QRImageProvider* qrProvider;
};

#endif // QRMANAGER_H
