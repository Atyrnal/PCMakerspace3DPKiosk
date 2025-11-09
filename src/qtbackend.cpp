/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/

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

//called from QML fileDialog
Q_INVOKABLE void QTBackend::fileUploaded(const QUrl &fileUrl) {
    QString filepath = fileUrl.toLocalFile(); //get filepath from url
    QMap<QString, QString> properties = GCodeParser::parseFile(filepath); //parse the gcode
    qDebug() << properties;
    QVariantMap propertiesForJS; //convert properties to QVariantMap for QML
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        propertiesForJS.insert(it.key(), it.value());
    }
    /*for (int p = 0; p < properties.size(); p++) {
        propertiesForJS.insert(properties.keys()[p], properties[properties.keys()[p]]);
    }*/

    //emit signals to main loop and QML to update appstate and load print files
    emit printInfoLoaded(propertiesForJS);
    emit printLoaded(filepath, properties);
}

Q_INVOKABLE void QTBackend::helpButtonClicked() {
    qDebug() << "Help! clicked." << Qt::endl;
}

Q_INVOKABLE void QTBackend::orcaButtonClicked() { //Runs when orcaslicer button pressed
    //Locate orcaslicer exe
    QString exeName = "orca-slicer.exe";
    QString exePath = "C:/Program Files/OrcaSlicer/orca-slicer.exe";

    //Find orcaslicer process
    DWORD pid = findProcessId(exeName);

    if (pid != 0) { //if process running, bring it to front instead of starting a new oen
        qDebug() << "OrcaSlicer is already running, bringing to front.";
        bringWindowToFront(pid);
    } else if (QFile::exists(exePath)){ //Otherwise launch it (if it is installed)
        qDebug() << "Launching OrcaSlicer...";
        QProcess::startDetached(exePath);
    } else {
        qWarning() << "OrcaSlicer installation not found";
    }
}

DWORD QTBackend::findProcessId(const QString& exeName) { //Windows shenanigans to find process by exename
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); //ProcessScanner

    if (Process32First(snapshot, &entry)) { //Scan Processes until we find one that matches
        do {
            if (QString::fromWCharArray(entry.szExeFile).compare(exeName, Qt::CaseInsensitive) == 0) { //if exename matches
                DWORD pid = entry.th32ProcessID;
                CloseHandle(snapshot); //Stop scanner
                return pid;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0; //return 0 if not found / not open
}

void QTBackend::bringWindowToFront(DWORD pid) {
    HWND hwnd = nullptr;
    HWND mainHwnd = nullptr;

    //Scan instances of specified process for one with a window
    while ((hwnd = FindWindowEx(nullptr, hwnd, nullptr, nullptr))) {
        DWORD windowPid = 0;
        GetWindowThreadProcessId(hwnd, &windowPid);

        // Skip windows not belonging to target process
        if (windowPid != pid)
            continue;

        // Skip invisible windows
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
        ShowWindow(mainHwnd, SW_RESTORE);//Show the window
        SetForegroundWindow(mainHwnd); //Bring it to front
        qDebug() << "Brought OrcaSlicer main window to front.";
    } else {
        qDebug() << "Could not find a visible top-level OrcaSlicer window.";
    }
}

