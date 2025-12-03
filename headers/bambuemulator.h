#ifndef BAMBUEMULATOR_H
#define BAMBUEMULATOR_H

#include <QObject>
#include <QString>
#include <QProcess>
#include "headers/bambulab.h"

class BambuEmulator : public QObject {
    Q_OBJECT
public:
    BambuEmulator(QObject* parent = nullptr);
    ~BambuEmulator();
    void addPrinter(quint32 id, BambuLab* printer);
    void removePrinter(quint32 id);
signals:
    void jobLoaded(quint32 id, const QString &filepath, QMap<QString, QString> properties);
private:
    QMap<quint32, BambuLab*> printers;
    QProcess* mosquito;
};

#endif // BAMBUEMULATOR_H
