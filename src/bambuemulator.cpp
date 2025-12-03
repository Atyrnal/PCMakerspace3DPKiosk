#include "headers/bambuemulator.h"

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
BambuEmulator::BambuEmulator(QObject* parent) : QObject(parent) {
    mosquito = new QProcess(parent);

    QString mosquitoPath = "C:/Program Files/mosquitto/mosquitto.exe";
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("mosq.conf");

    mosquito->start(mosquitoPath, QStringList() << "-c" << configPath << "-v");
    if (!mosquito->waitForStarted(2000)) {
        qCritical() << "Failed to start mosquito";
        return;
    };

    QObject::connect(mosquito, &QProcess::readyReadStandardOutput, this, [this](){
        qDebug() << "[mosq]" << this->mosquito->readAllStandardOutput();
    });
    QObject::connect(mosquito, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [](int code, QProcess::ExitStatus){ qDebug() << "mosquitto exited" << code; });
}

void BambuEmulator::addPrinter(quint32 id, BambuLab* printer) {
    printers.insert(id, printer);
}

void BambuEmulator::removePrinter(quint32 id) {
    printers.remove(id);
}

BambuEmulator::~BambuEmulator() {
    mosquito->terminate();
}
