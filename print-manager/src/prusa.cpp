/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/


#include "prusa.h"
#include <QHttpMultiPart>
#include <QFileInfo>
#include <QJsonObject>
#include "errors.hpp"
#include <QTimer>

Prusa::Prusa(QObject* parent) : Printer(parent) {
    //connect(&manager, &QNetworkAccessManager::authenticationRequired, this, &PrusaLink::provideAuth);
    testConnection();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Prusa::testConnection);
    timer->start(10000);
}

Prusa::Prusa(QString name, QString model, QString hostname, QString apiKey, QString storageType, QObject* parent) : Printer(name, model, "Prusa", parent) {
    this->hostname = hostname;
    this->apiKey = apiKey;
    this->storageType = storageType;
    testConnection();
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this](){
        if (connectedOnce) testConnection();
    });
    timer->start(10000);
}


void Prusa::setHostname(QString hostname) {
    this->hostname = hostname;
}

void Prusa::setApiKey(QString apiKey) {
    this->apiKey = apiKey;
}

void Prusa::setStorageType(QString storageType) {
    this->storageType = storageType;
}

void Prusa::startPrint(const QString &fileName) {
    sendGCode(fileName);
}

Printer::JobStatus Prusa::getJobStatus() {
    //TODO: Implement proabably? Or make it so that status is only checked upon selection??? idk
    return Printer::JobStatus::Idle;
}

/*bool Prusa::testConnection() {
    QUrl verUrl(QString("http://%1/api/version").arg(hostname));
    QNetworkRequest verReq(verUrl);
    verReq.setRawHeader("X-Api-Key", apiKey.toUtf8());

    QNetworkReply* verReply = manager.get(verReq);

}*/

void Prusa::sendGCode(QString filepath) {
    Log::write("PrusaPrinter("+name+"@"+hostname+")", "Attempting to start print " + filepath);
    //Read gcode file
    QFileInfo fileInfo(filepath);
    QFile file = QFile(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        Error::handle("PrusaPrinterUploadError", "Cannot open file for upload: " + filepath);
        return;
    }
    QByteArray fileData = file.readAll();
    file.close();

    //upload the file to the printer via its API
    QUrl uploadUrl(QString("http://%1/api/v1/files/%2/%3").arg(hostname, storageType, fileInfo.fileName()));
    QNetworkRequest uploadReq(uploadUrl);
    uploadReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    uploadReq.setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(fileData.size()));
    uploadReq.setRawHeader("X-Api-Key", apiKey.toUtf8());
    uploadReq.setRawHeader("Print-After-Upload", "?1"); //Ensure the print starts imidately
    uploadReq.setRawHeader("Overwrite", "?1");


    QNetworkReply *uploadReply = manager.put(uploadReq, fileData);

    //When uploadreply recieved
    QObject::connect(uploadReply, &QNetworkReply::finished, uploadReply, [=, this]() {
        //Log if the upload succeeded or failed
        if (uploadReply->error() != QNetworkReply::NoError) {
            Error::handle("PrusaPrinterUploadError", uploadReply->errorString());
            uploadReply->deleteLater();
            return;
        }
        //QByteArray resp = uploadReply->readAll();
        Log::write("PrusaPrinter("+name+"@"+hostname+")", "Print upload succeeded");
        uploadReply->deleteLater();
    });
}

void Prusa::testConnection() {
    QUrl testUrl(QString("http://%1/api/v1/info").arg(hostname));
    QNetworkRequest testReq(testUrl);
    testReq.setRawHeader("X-Api-Key", apiKey.toUtf8());
    QNetworkReply* testReply = manager.get(testReq);
    QObject::connect(testReply, &QNetworkReply::finished, this, [this, testReply]() {
        if (testReply->error() != QNetworkReply::NoError) {
            if (testReply->error() >= 100) Error::softHandle("PrusaPrinterConnectionTestError", testReply->errorString() + " " + QString::number(testReply->error()));
            if (connectionStatus) {
                emit this->connectionUpdated(false);
                Log::write("PrusaPrinter("+name+"@"+hostname+")", "Disconnected from printer");
            } else if (!connectedOnce) {
                Error::handle("PrusaPrinterConnectionTestError", "Unable to connect to printer " + name, El::Warning);
            }
            connectionStatus = false;
            return;
        }
        //Success
        if (!connectionStatus) {
            Log::write("PrusaPrinter("+name+"@"+hostname+")", "Connected successfully");
            emit this->connectionUpdated(true);
        }
        connectionStatus = true;
        connectedOnce = true;
        return;
    });
    QTimer::singleShot(10000, this, [this, testReply](){
        if (!testReply->isFinished()) {
            if (connectionStatus) {
                emit this->connectionUpdated(false);
                Log::write("PrusaPrinter("+name+"@"+hostname+")", "Disconnected from printer");
            } else if (!connectedOnce) {
                Error::handle("PrusaPrinterConnectionTestError", "Unable to connect to printer " + name, El::Warning);
            }
            connectionStatus = false;
        }
        testReply->deleteLater();
    });
}

/*void PrusaLink::sendGCode(QString filepath, QString hostname, QString password, QString storageName) {


    QUrl uploadUrl(QString("http://%1/api/v1/storage/%2/upload").arg(PRUSALINK_HOSTNAME, storageName));
    QNetworkRequest uploadReq(uploadUrl);
    uploadReq.setRawHeader("X-Api-Key", password.toUtf8());

    QHttpMultiPart *multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"" + QFileInfo(filepath).fileName() + "\""));
    QFile *file = new QFile(filepath);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file for upload:" << filepath;
        delete multi;
        delete file;
        return;
    }
    filePart.setBodyDevice(file);

    multi->append(filePart);

    QNetworkReply *uploadReply = manager.put(uploadReq, multi);
    multi->setParent(uploadReply);
    file->setParent(multi);

    QObject::connect(uploadReply, &QNetworkReply::finished, uploadReply, [=]() {
        if (uploadReply->error() != QNetworkReply::NoError) {
            qWarning() << "Upload failed:" << uploadReply->errorString();
            uploadReply->deleteLater();
            return;
        }
        QByteArray resp = uploadReply->readAll();
        qDebug() << "Upload succeeded response:" << resp;

        // 2) Start the print job
        QUrl printUrl(QString("http://%1/api/v1/job").arg(hostname));
        QNetworkRequest printReq(printUrl);
        printReq.setRawHeader("X-Api-Key", password.toUtf8());
        printReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject bodyObj;
        bodyObj["storage"] = storageName;
        bodyObj["filename"] = QFileInfo(filepath).fileName();
        // bodyObj["startImmediately"] = true;

        QJsonDocument doc(bodyObj);
        QByteArray body = doc.toJson();

        QNetworkReply *printReply = manager.post(printReq, body);
        QObject::connect(printReply, &QNetworkReply::finished, [=]() {
            if (printReply->error() != QNetworkReply::NoError) {
                qWarning() << "Start print failed:" << printReply->errorString();
            } else {
                qDebug() << "Print started, response:" << printReply->readAll();
            }
            printReply->deleteLater();
        });

        uploadReply->deleteLater();
    });
}*/

/*void PrusaLink::provideAuth(QNetworkReply *, QAuthenticator *authenticator) {
    authenticator->setUser(PRUSALINK_USERNAME);        // your username
    authenticator->setPassword(PRUSALINK_PASSWORD); // your password
}*/
