#ifndef QTBACKEND_H
#define QTBACKEND_H

#include <QObject>
#include <QFile>

#ifdef Q_OS_WIN
//#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#endif

class QTBackend : public QObject {
    Q_OBJECT
public:
    explicit QTBackend(QObject* parent = 0);
    void showMessage(QObject* root, QString message, QString acceptText="OK", const int &redirectState = 0);
private:
    DWORD findProcessId(const QString &processName);
    void bringWindowToFront(DWORD pid);
signals:
    void printLoaded(const QString &gcodeFilepath, const QMap<QString, QString> &printInfo);
    void printInfoLoaded(const QVariantMap &printInfo);
    void messageReq(const QString &message, const QString &buttonText, const int &redirectState);
public slots:
    Q_INVOKABLE void orcaButtonClicked();
    Q_INVOKABLE void helpButtonClicked();
    Q_INVOKABLE void fileUploaded(const QUrl &fileUrl);
};

#endif // QTBACKEND_H
