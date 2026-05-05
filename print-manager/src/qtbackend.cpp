/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/

#include "qtbackend.h"
#include <QProcess>
#include <QDebug>
#include <QUrl>
#include "gcodeparser.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>
#include <QQmlContext>
#include "errorhandler.hpp"
#include "ltx2aQT.h"
#include <QCoreApplication>

#ifndef Q_OS_WIN
#include <QStandardPaths>
#endif

#define PRINTING_CERT_ID "recY34WO6fex1KMxO"


QTBackend::QTBackend(QCoreApplication* app, QQmlApplicationEngine* eng, QObject* parent) : QObject(parent) {
    ErrorHandler::bk = this;

    rfidReader.start(); //Initialize the RFID reader
    QObject::connect(app, &QCoreApplication::aboutToQuit, &rfidReader, &LTx2A::stop); //Connect the aboutToQuit app event to the rfidReader's stop function

    pm = new PrinterManager(parent);

    engine = eng;
    engine->rootContext()->setContextProperty("backend", this);
    engine->rootContext()->setContextProperty("printermanager", pm);
    engine->rootContext()->setContextProperty("printersModel", pm->getModel());
    #ifdef Q_OS_WIN
    engine->rootContext()->setContextProperty("isWindows", true);
    #else
    engine->rootContext()->setContextProperty("isWindows", false);
    #endif

    connect(this, &QTBackend::printLoaded, this, &QTBackend::jobLoaded);
    connect(pm, &PrinterManager::jobLoaded, this, &QTBackend::jobLoaded);

    connect(app, &QCoreApplication::aboutToQuit, pm, &PrinterManager::closing);


    QObject::connect(&rfidReader, &LTx2A::cardScanned, this, [this]() { //Connect the rfidReader cardScanned event to the lambda
        if (rfidReader.hasNext()) { //If the cards scanned queue is not empty
            QString cardid = rfidReader.getNext().replace("\"", "").trimmed();
            this->cardScanned(cardid);
        }
    });



}


//Utility functions

double QTBackend::parseDuration(const QString &durationString) {
    //Regex pattern to extract numbers from string
    static QRegularExpression regex(R"((?:(\d+(?:\.\d+)?)h)?\s*(?:(\d+(?:\.\d+)?)m)?\s*(?:(\d+(?:\.\d+)?)s)?)");
    QRegularExpressionMatch match = regex.match(durationString);

    if (!match.hasMatch()) return 999.99; //Max time if string is invalid so print doesnt go through
    double totalHours = 0.0;
    if (match.captured(1).length() > 0)  // hours
        totalHours += match.captured(1).toDouble();
    if (match.captured(2).length() > 0)  // minutes
        totalHours += match.captured(2).toDouble() / 60.0;
    if (match.captured(3).length() > 0)  // seconds
        totalHours += match.captured(3).toDouble() / 3600.0;
    return totalHours;
}

#ifdef Q_OS_WIN
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
        Log::write("QtBackend", "Brought OrcaSlicer main window to front");
    } else {
        Error::handle("QtBackendError", "Could not find OrcaSlicer window", El::Trivial);
    }
}
#endif

AppState QTBackend::appstate() {
    bool ok = true;
    int prop = root->property("appstate").toInt(&ok);
    if (!ok) {
        Error::softHandle("QtBackendQMLError", "Appstate is not int", El::Warning);
        return AppState::Idle;
    }
    return static_cast<AppState>(prop);
}


//QML accessible functions

Q_INVOKABLE void QTBackend::fileUploaded(const QUrl &fileUrl) {
    QString filepath = fileUrl.toLocalFile(); //get filepath from url
    Log::write("QtBackend", "File uploaded: " + filepath);
    Eo<QMap<QString, QString>> propertiesEo = GCodeParser::parseFile(filepath); //parse the gcode

    //qDebug() << propertiesEo;
    if (propertiesEo.isError()) {
        return propertiesEo.handle();
    }
    auto properties = propertiesEo.get();
    QVariantMap propertiesForJS; //convert properties to QVariantMap for QML
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        propertiesForJS.insert(it.key(), it.value());
    }
    if (pm->getPrinter(loadedPrint.printerId) != nullptr) propertiesForJS.insert("printerName", pm->getPrinter(loadedPrint.printerId)->getName());
    Log::write("QtBackend", "Loaded print info for: " + filepath);
    //emit signals to main loop and QML to update appstate and load print files
    emit printInfoLoaded(propertiesForJS);
    emit printLoaded(loadedPrint.printerId, filepath, properties);
}

Q_INVOKABLE void QTBackend::helpButtonClicked() {

}

Q_INVOKABLE void QTBackend::setLoadedPrintFilamentProvider(bool personal) {
    this->loadedPrint.isPersonalFilament = personal;
}


Q_INVOKABLE void QTBackend::orcaButtonClicked() { //Runs when orcaslicer button pressed
    //Locate orcaslicer exe
    #ifdef Q_OS_WIN
    QString exeName = "orca-slicer.exe";
    QString exePath = "C:/Program Files/OrcaSlicer/orca-slicer.exe";

    //Find orcaslicer process

    DWORD pid = findProcessId(exeName);

    if (pid != 0) { //if process running, bring it to front instead of starting a new oen
        Log::write("QtBackend", "Bringing running OrcaSlicer instance to front");
        bringWindowToFront(pid);
    } else if (QFile::exists(exePath)){ //Otherwise launch it (if it is installed)
        Log::write("QtBackend", "Launching OrcaSlicer instance");
        QProcess::startDetached(exePath);
    } else {
        Error::handle("QtBackendError", "OrcaSlicer installation not found", El::Warning);
    }
    #else
    QString exe = QStandardPaths::findExecutable("orca-slicer");
    if (exe.isEmpty())
        exe = QStandardPaths::findExecutable("OrcaSlicer");
    if (exe.isEmpty())
        exe = QStandardPaths::findExecutable("orcaslicer");
    if (exe.isEmpty())
        exe = "/opt/orca-slicer/bin/orca-slicer";
        //exe = "/usr/bin/orca-slicer"; // fallback
    if (QFile::exists(exe)){ //Otherwise launch it (if it is installed)
        Log::write("QtBackend", "Launching OrcaSlicer instance");
        QProcess::startDetached(exe);
    } else {
        Error::handle("QtBackendError", "OrcaSlicer installation not found", El::Warning);
    }
    #endif

}


//Qt accessible functions

void QTBackend::setRoot(QObject* r) {
    root = r;
}

void QTBackend::loadConfig(QJsonObject cfg) {
    config = cfg;
    pm->loadConfig(cfg);
    //TODO: Implement some sort of abstraction to allow program to work with any database
    if (!config.contains("airtable") || !config.value("airtable").isObject()) return Error("ConfigError", "Missing Airtable Credentials", El::Fatal).handle();
    QJsonObject airtablec = config.value("airtable").toObject();
    if (!airtablec.contains("hostname") || !airtablec.contains("key") || !airtablec.contains("base") || !airtablec.value("hostname").isString() || !airtablec.value("key").isString() || !airtablec.value("base").isString()) return Error("ConfigError", "Missing Airtable Credentials", El::Fatal).handle();
    airtable = new AirtableBase(airtablec.value("hostname").toString(), airtablec.value("key").toString(), airtablec.value("base").toString(), this);
}

void QTBackend::showMessage(QString message, QString acceptText, int redirectState) {
    emit messageReq(message, acceptText, redirectState);
    root->setProperty("appstate", AppState::Message);
}

void QTBackend::jobLoaded(quint32 id, const QString &filepath, const QMap<QString, QString> &printInfo) {
    this->loadedPrint = LoadedPrint();
    loadedPrint.filepath = filepath; //set filepath
    loadedPrint.printInfo = printInfo; //set printinfo
    loadedPrint.printerId= id;
    root->setProperty("appstate", AppState::Prep); //change QML appstate to show print info
}

void QTBackend::cardScanned(const QString &cardid) {
    Log::write("QtBackendSerial", "Card scanned, current state:" + QString::number(appstate()));
    if (appstate() == AppState::UserScan) {
        currentUserID = cardid;
        loadedPrint.userID = cardid;
        Log::write("QtBackendSerial", "User card scanned: " + cardid);
        root->setProperty("appstate", AppState::Loading);
        airtable->table("Users")->getRecord(QString("{User Id} = '%1'").arg(currentUserID), [=, this](Eo<QVariantMap> recordeo){
            if (recordeo.isError()) {
                if (recordeo.errorLevel() <= El::Trivial) {
                    recordeo.softHandle();
                    return showMessage("Your account is not Registered\nPlease Register at the Check-In Kiosk");
                } else {
                    return recordeo.handle();
                }
            }
            currentUser = recordeo.get();
            QVariantMap recordFields = recordeo.get().value("fields").toMap(); //Could need toJsonObject instead?
            bool isStaff = recordFields.value("Is Staff", false).toBool();
            if (isStaff) currentStaffID = cardid;
            printStartCheck(isStaff);
        });
    } else if (appstate() == AppState::StaffScan) {

        //Staff scan to confirm a user has completed 3D Printing training

        //Get staff info
        root->setProperty("appstate", AppState::Loading);
        airtable->table("Users")->getRecord(QString("{User Id} = '%1'").arg(cardid), [=, this](Eo<QVariantMap> recordeo){
            if (recordeo.isError()) {
                if (recordeo.errorLevel() <= El::Trivial) {
                    recordeo.softHandle();
                    return showMessage("Your account is not Registered\nPlease Register at the Check-In Kiosk");
                } else {
                    return recordeo.handle();
                }
            }
            QVariantMap recordFields = recordeo.get().value("fields").toMap(); //Could need toJsonObject instead?
            bool isStaff = recordFields.value("Is Staff", false).toBool();
            if (!isStaff) return showMessage("This user is not Staff\nPlease ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);
            currentStaffID = cardid;
            if (!currentUser.contains("id")) return;
            QVariantMap payload;
            QVariantList certificates = currentUser.value("fields", QVariantMap()).toMap().value("Certificates", QList<QString>()).toList();
            certificates.append(PRINTING_CERT_ID);
            payload.insert("Certificates", certificates);

            airtable->table("Users")->updateRecordById(currentUser.value("id").toString(), payload);

            printStartCheck(false, true);
        });

    } else if (appstate() == AppState::Message) {
        //Staff override scan
        QVariantMap propertiesForJS; //convert properties to QVariantMap for QML
        for (auto it = loadedPrint.printInfo.constBegin(); it != loadedPrint.printInfo.constEnd(); ++it) {
            propertiesForJS.insert(it.key(), it.value());
        }
        propertiesForJS.insert("personalFilament", loadedPrint.isPersonalFilament);
        if (pm->getPrinter(loadedPrint.printerId) != nullptr) propertiesForJS.insert("printerName", pm->getPrinter(loadedPrint.printerId)->getName());
        emit printInfoLoaded(propertiesForJS);
        root->setProperty("appstate", AppState::Prep);
    } else if (appstate() == AppState::Prep && loadedPrint.userID != "") {
        root->setProperty("appstate", AppState::Loading);
        airtable->table("Users")->getRecord(QString("{User Id} = '%1'").arg(cardid), [=, this](Eo<QVariantMap> recordeo){
            if (recordeo.isError()) {
                if (recordeo.errorLevel() <= El::Trivial) {
                    recordeo.softHandle();
                    return;
                } else {
                    return recordeo.handle();
                }
            }
            QVariantMap recordFields = recordeo.get().value("fields").toMap(); //Could need toJsonObject instead?
            bool isStaff = recordFields.value("Is Staff", false).toBool();
            if (!isStaff) return;
            currentStaffID = cardid;
            printStartCheck(true);
        });
    } else return;    //If we're not in one of the scan states, ignore the card scan

    //This code executes when a print is verified and authorized
     //show printing message

    //Update user print statistics

    // auto response = queryDatabase("SELECT printsStarted, filamentUsedGrams, printHours FROM users WHERE id = :id LIMIT 1", {{":id", currentUserID}});
    // if (response.isError()) {
    //     ErrorHandler::softHandle(response);
    // } else {

    //     QMap<QString, QVariant> result = response.get();
    //     //Increase stats as needed
    //     int printsStarted = result.value("printsStarted").toInt();
    //     double filamentUsed = result.value("filamentUsedGrams").toDouble();
    //     double printHours = result.value("printHours").toDouble();
    //     printsStarted++;
    //     filamentUsed+=loadedPrintInfo["weight"].toDouble();
    //     printHours+=parseDuration(loadedPrintInfo["duration"]);

    //     //Update stats to database
    //     auto response2 = queryDatabase("UPDATE users SET printsStarted = :ps, filamentUsedGrams = :fu, printHours = :ph WHERE id = :id;", {
    //         {":ps", printsStarted},
    //         {":fu", filamentUsed},
    //         {":ph", printHours},
    //         {":id", currentUserID}
    //     });
    //     if (response2.isError()) {
    //         ErrorHandler::softHandle(response2);
    //     }
    // }

    //Send print log to database
    // auto response3 = queryDatabase("INSERT INTO printLog (printerName, durationHours, weight, printer, user, filament, filename, timestamp) "
    //                                "VALUES(:pn, :dh, :wt, :pr, :us, :fm, :fn, :tm)", {
    //                                 {":pn", pm->getPrinter(loadedPrinterId)->getName()},
    //                                 {":dh", parseDuration(loadedPrintInfo["duration"])},
    //                                 {":wt", loadedPrintInfo["weight"].toDouble()},
    //                                 {":pr", loadedPrintInfo["printer"]},
    //                                 {":us", currentUserID},
    //                                 {":fm", loadedPrintInfo["filamentType"]},
    //                                 {":fn", loadedPrintInfo["filename"]},
    //                                 {":tm", QString("%1").arg(QDateTime::currentSecsSinceEpoch())}
    //                                });
    // if (response3.isError()) {
    //     ErrorHandler::softHandle(response3);
    // }

    //Send the print to the printer

}

void QTBackend::printStartCheck(bool staffApproved, bool justTrained) {
    double printDuration = parseDuration(loadedPrint.printInfo["duration"]);
    if (!staffApproved) {
        QVariantMap recordFields = currentUser.value("fields").toMap();
        QString cicsAff = recordFields.value("Affiliation to CICS", "").toString();
        bool isCICS = cicsAff == "CICS Student" || cicsAff == "CICS Faculty or Staff";
        bool training = justTrained || recordFields.value("Certificates", QList<QString>()).toList().contains(PRINTING_CERT_ID); //Need to implement some form of join or something idek



        if (!isCICS && !loadedPrint.isPersonalFilament) return showMessage("Sorry, but only CICS Community Members\nmay print using Makerspace filament.", "I Understand");
        if (!training) return showMessage("Please ask a staff member to\napprove your print or take \nour 3D Print training", "Training Completed", AppState::StaffScan);
        if (!staffApproved && printDuration > 6.0 && !loadedPrint.isPersonalFilament) return showMessage("Prints cannot be longer than 6 hours\nwith Makerspace Filament");
        if (!staffApproved && printDuration > 10.0 && loadedPrint.isPersonalFilament && isCICS) return showMessage("Prints cannot be longer than 10 hours\n");
        if (!staffApproved && printDuration > 6.0 && loadedPrint.isPersonalFilament && !isCICS) return showMessage("Prints cannot be longer than 6 hours\n");
    }
    showMessage("Printing now!");
    if (pm->getPrinter(loadedPrint.printerId) == nullptr) return Error("QTBackendError", "Loaded Printer not found", El::Critical).handle();
    QVariantMap printLogInfo = {
        {"User ID", currentUserID},
        {"Printer", pm->getPrinter(loadedPrint.printerId)->getName()},
        {"Printer Model", pm->getPrinter(loadedPrint.printerId)->getBrand() + " " + pm->getPrinter(loadedPrint.printerId)->getModel()},
        {"Weight", loadedPrint.printInfo["weight"].left(loadedPrint.printInfo["weight"].size()-1).toDouble()}, //Remove the g and convert to double
        {"Duration", printDuration*3600},
        {"Filament Type", (loadedPrint.printInfo.contains("filament") && loadedPrint.printInfo["filament"] != "") ? loadedPrint.printInfo["filament"] : loadedPrint.printInfo["filamentType"]},
        {"Filename", loadedPrint.printInfo["filename"]},
        {"Personal Filament", loadedPrint.isPersonalFilament},
    };
    if (staffApproved) printLogInfo.insert("Staff Approver ID", currentStaffID);
    airtable->table("Print Log")->createRecord(printLogInfo);
    Log::write("QTBackend", "Starting Print");
    pm->startPrint(loadedPrint.printerId, loadedPrint.filepath);
}

// Error QTBackend::queryDatabase(const QString &query) {
//     QSqlQuery q; //Create query
//     q.prepare(query); //Prepare query
//     if(q.exec()) { //execute the query and return result
//         return Error::None();
//     } else {
//         return Error("DatabaseQueryError", q.lastError().text(), El::Warning);
//     }
// }

// Eo<QMap<QString, QVariant>> QTBackend::queryDatabase(const QString &query, const QMap<QString, QVariant> &values) {
//     using eop = Eo<QMap<QString, QVariant>>;
//     QSqlQuery q; //Create query
//     q.prepare(query); //Prepare query
//     for (auto it = values.constBegin(); it != values.constEnd(); ++it) { //insert values (parameterized to prevent sql injection vulnerability)
//         q.bindValue((it.key().startsWith(":")) ? it.key() : ":" + it.key(), it.value());
//     }

//     QList bound = q.boundValues();
//     for (auto it = bound.constBegin(); it != bound.constEnd(); ++it) {
//         if (!it->isValid()) {
//             return eop("DatabaseQueryBindError", "Failed to bind values", El::Warning);
//         }
//     }
//     bool success = q.exec(); //execute the query
//     if (!success || !q.isActive()) {
//         return eop("DatabaseQueryError", q.lastError().text(), El::Warning);
//     }

//     if (!q.next() || !q.isValid()) {
//         return eop("DatabaseQueryError", "No valid record", El::Debug);
//     }

//     QMap<QString, QVariant> output;
//     QSqlRecord rec = q.record();
//     for (int i = 0; i < rec.count(); i++) {
//         output.insert(rec.fieldName(i), rec.value(i));
//     }

//     return eop(output);
// }

// Eo<QList<QMap<QString, QVariant>>> QTBackend::queryDatabaseMultirow(const QString &query, const QMap<QString, QVariant> &values) {
//     using eop = Eo<QList<QMap<QString, QVariant>>>;
//     QSqlQuery q; //Create query
//     q.prepare(query); //Prepare query
//     for (auto it = values.constBegin(); it != values.constEnd(); ++it) { //insert values (parameterized to prevent sql injection vulnerability)
//         q.bindValue((it.key().startsWith(":")) ? it.key() : ":" + it.key(), it.value());
//     }

//     QList bound = q.boundValues();
//     for (auto it = bound.constBegin(); it != bound.constEnd(); ++it) {
//         if (!it->isValid()) {
//             return eop("DatabaseQueryBindError", "Failed to bind values", El::Trivial);
//         }
//     }
//     bool success = q.exec(); //execute the query
//     if (!success || !q.isActive() || !q.isValid()) {
//         return eop("DatabaseQueryError", q.lastError().text(), El::Warning);
//     }

//     QList<QMap<QString, QVariant>> output;
//     while (q.next()) {
//         QSqlRecord rec = q.record();
//         QMap<QString, QVariant> r;
//         for (int i = 0; i < rec.count(); i++) {
//             r.insert(rec.fieldName(i), rec.value(i));
//         }
//         output.append(r);
//     }
//     return eop(output);
// }
