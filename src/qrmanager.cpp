#include "headers/qrmanager.h"

QRManager::QRManager(QRImageProvider* provider, QObject *parent) : QObject(parent), qrProvider(provider) {}

Q_INVOKABLE void QRManager::updateQRCode(const QString &data) { //Function to regenerate qr code and display it in ui
    qrProvider->setData(data); //Change the link in the qrprovider (will be used when image is regenerated)
    emit qrCodeChanged(); //Emit qrcodechanged event (listened to by the QML, which will call qrProvider and regenerate the qr code)
}
