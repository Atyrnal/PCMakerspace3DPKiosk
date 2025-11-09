#include "headers/qtbackend.h"
#include <QProcess>
#include <QDebug>
#include <QUrl>
#include "headers/gcodeparser.h"


QTBackend::QTBackend(QObject* parent) : QObject(parent) {}

void QTBackend::showMessage(QObject* root, QString message, QString acceptText, const int &redirectState) {
    emit messageReq(message, acceptText, redirectState);
    root->setProperty("appstate", 2);
}

Q_INVOKABLE void QTBackend::fileUploaded(const QUrl &fileUrl) {
    QString filepath = fileUrl.toLocalFile();
    QMap<QString, QString> properties = GCodeParser::parseFile(filepath);
    qDebug() << properties;
    QVariantMap propertiesForJS;
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        propertiesForJS.insert(it.key(), it.value());
    }
    /*for (int p = 0; p < properties.size(); p++) {
        propertiesForJS.insert(properties.keys()[p], properties[properties.keys()[p]]);
    }*/
    emit printInfoLoaded(propertiesForJS);
    emit printLoaded(filepath, properties);
}

Q_INVOKABLE void QTBackend::helpButtonClicked() {
    qDebug() << "Help! clicked." << Qt::endl;
}

Q_INVOKABLE void QTBackend::orcaButtonClicked() {
    QString exeName = "orca-slicer.exe";
    QString exePath = "C:/Program Files/OrcaSlicer/orca-slicer.exe";

    DWORD pid = findProcessId(exeName);

    if (pid != 0) {
        qDebug() << "OrcaSlicer is already running, bringing to front.";
        bringWindowToFront(pid);
    } else if (QFile::exists(exePath)){
        qDebug() << "Launching OrcaSlicer...";
        QProcess::startDetached(exePath);
    } else {
        qWarning() << "OrcaSlicer installation not found";
    }
}

DWORD QTBackend::findProcessId(const QString& exeName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        do {
            if (QString::fromWCharArray(entry.szExeFile).compare(exeName, Qt::CaseInsensitive) == 0) {
                DWORD pid = entry.th32ProcessID;
                CloseHandle(snapshot);
                return pid;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

void QTBackend::bringWindowToFront(DWORD pid) {
    HWND hwnd = nullptr;
    HWND mainHwnd = nullptr;

    while ((hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr))) {
        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);

        // Skip windows not belonging to OrcaSlicer
        if (windowPid != pid)
            continue;

        // Skip invisible or minimized windows
        if (!IsWindowVisible(hwnd))
            continue;

        // Skip child/owned windows (dialogs, popups)
        if (GetWindow(hwnd, GW_OWNER) != nullptr)
            continue;

        // Found a visible, top-level window for this process
        mainHwnd = hwnd;
        break;
    }

    if (mainHwnd) {
        ShowWindow(mainHwnd, SW_RESTORE);
        SetForegroundWindow(mainHwnd);
        qDebug() << "Brought OrcaSlicer main window to front.";
    } else {
        qDebug() << "Could not find a visible top-level OrcaSlicer window.";
    }
}

