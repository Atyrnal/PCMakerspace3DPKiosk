#ifndef BAMBUEMULATOR_H
#define BAMBUEMULATOR_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QHttpServer>
#include <QUdpSocket>
#include "headers/bambulab.h"
#include "headers/errors.hpp"

class BambuEmulator : public QObject {
    Q_OBJECT
public:
    BambuEmulator(QObject* parent = nullptr);
    ~BambuEmulator();
    void addPrinter(quint32 id, BambuLab* printer);
    void removePrinter(quint32 id);
    void closing();
signals:
    void jobLoaded(quint32 id, const QString &filepath, QMap<QString, QString> properties);
private:
    QMap<QString, BambuLab*> printers;
    QMap<quint32, QString> SNs;
    QProcess* mosquito;
    QMqttClient* mqtt;
    void slicerHandshake(QSslSocket* socket, const QString &vSN, BambuLab* printer);
    Error fetchPrinterInfo(BambuLab* printer);
};
#endif // BAMBUEMULATOR_H
