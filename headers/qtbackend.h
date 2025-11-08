#ifndef QTBACKEND_H
#define QTBACKEND_H

#include <QObject>
#ifdef Q_OS_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#endif

class QTBackend : public QObject {
    Q_OBJECT
public:
    explicit QTBackend(QObject* parent = 0);
private:
    DWORD findProcessId(const QString &processName);
    void bringWindowToFront(DWORD pid);
signals:

public slots:
    Q_INVOKABLE void orcaButtonClicked();
    Q_INVOKABLE void helpButtonClicked();
};

#endif // QTBACKEND_H
