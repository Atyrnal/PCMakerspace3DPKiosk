#include "headers/qrprovider.h"
#include "headers/qrcodegen.hpp"
#include <QtSvg/QSvgRenderer>
#include <QPainter>

//Purpose of this class is to create qr codes from data and provide them dynamically to the QML ui

using namespace qrcodegen;

QRImageProvider::QRImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

void QRImageProvider::setData(const QString &data) { //Set the qr link
    currentData = data;
}

QImage QRImageProvider::generateQRCode(const QString &data, const QSize &requestedSize) { //Clusterfuck
    //generates a qr code for a link at a requested size (except requestedSize is always 0 for some reason so we dont use it)
    QrCode qr = QrCode::encodeText(data.toStdString().c_str(), QrCode::Ecc::LOW); //Use qrcodegen library to create a qrcode object with given data
    std::string qrsvg = QrCode::toSvgString(qr, 1); //Convert that qrcode object into a string of svg xml
    QByteArray byteArray(qrsvg.c_str(), static_cast<int>(qrsvg.size())); //Convert the svg string into bytes
    QSvgRenderer renderer(byteArray); //Create an svg renderer from the svg string bytes
    QImage image(requestedSize.isValid() ? requestedSize : QSize(600,600), QImage::Format_ARGB32); //Create a blank image of requested size (600, 600)
    image.fill(Qt::white); //Fill it with white pixels
    QPainter painter(&image); //Create a painter on the image to access pixels
    renderer.render(&painter); //Paint the image with the rendered svg data
    //painter.end();
    return image; //return the finished qr code image
}

QImage QRImageProvider::requestImage(const QString &id, QSize* size, const QSize & requestedSize) { //Called by the QML whenever qrManager updates the qr data
    Q_UNUSED(id) //Ignoring the id because we use currentData instead
    QImage qr = generateQRCode(currentData, requestedSize); //Create the qr image
    if (size) *size = qr.size(); //Let the program know the final image size
    return qr; //return the qrimage to the QML for rendering
}

