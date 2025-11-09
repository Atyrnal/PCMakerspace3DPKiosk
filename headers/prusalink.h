/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/

#ifndef PRUSALINK_H
#define PRUSALINK_H

#include <QObject>
#include <QFile>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QNetworkAccessManager>

class PrusaLink : public QObject {
    Q_OBJECT
public:
    PrusaLink(QObject* parent = 0);
    void startPrint(const QString &gcodeFilepath);
signals:

public slots:

private slots:
    // provideAuth(QNetworkReply *, QAuthenticator *authenticator);
private:
    QNetworkAccessManager manager;
    void sendGCode(QString filepath, QString hostname, QString password, QString storageType);
};


#endif // PRUSALINK_H
