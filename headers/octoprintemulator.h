#ifndef OCTOPRINTEMULATOR_H
#define OCTOPRINTEMULATOR_H

#include <QObject>
#include <QHttpServer>
#include <QFileInfo>

class OctoprintEmulator : public QObject {
    Q_OBJECT
public:
    OctoprintEmulator(QObject* parent = 0);
signals:
    void jobLoaded(const QString &filepath, const QMap<QString, QString> &properties);
    void jobInfoLoaded(const QVariantMap &properties);
private:
    QHttpServer server;
    QFileInfo* fileInfo = nullptr;
};

#endif // OCTOPRINTEMULATOR_H
