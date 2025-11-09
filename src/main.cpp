/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "headers/ltx2aQT.h"
#include <QTimer>
#include <QQmlContext>
#include <QRegularExpression>
#include "headers/qtbackend.h"
#include "headers/prusaLink.h"
#include "headers/octoprintemulator.h"
#include <QQuickWindow>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMap>

//Atyrnal 10/29/2025

//See ui/Main.qml for ui declarations

LTx2A rfidReader;

enum AppState {
    Idle,
    Prep,
    Message,
    UserScan,
    StaffScan,
    Printing,
    Loading
};

bool queryDatabase(const QString &query, const QMap<QString, QVariant> &values) {
    QSqlQuery q; //Create query
    q.prepare(query); //Prepare query
    QStringList keys = values.keys();
    for (int i = 0; i < values.size(); i++) { //insert values (parameterized to prevent sql injection vulnerability)
        QString k = keys[i];
        QVariant v = values.value(k);
        q.bindValue(k, v);
    }
    return q.exec(); //execute the query and return result
}

double parseDuration(QString durationString) {
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

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv); //Create a QT gui app

    //Connect to SQLITE DataBase
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("kioskdb.db");

    if (!db.open()) { //Ensure DB connection
        qCritical() << "Failed to connect to database:" << db.lastError().text();
        return -1;
    }

    rfidReader.start(); //Initialize the RFID reader
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &rfidReader, &LTx2A::stop); //Connect the aboutToQuit app event to the rfidReader's stop function

    QQmlApplicationEngine engine; //Qt engine creation
    QObject::connect( //Exit if object creation fails
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    //Helper classes
    QTBackend bk;
    PrusaLink pl;
    OctoprintEmulator ope;

    //Set the helpers to be accesible from QML
    engine.rootContext()->setContextProperty("backend", &bk);
    engine.rootContext()->setContextProperty("octoprintemulator", &ope);

    engine.loadFromModule("PCMakerspace3DPKiosk", "Main"); //Load the QML Main.qml declarative ui file

    //App state variables
    QFile* loadedPrint = nullptr; //Selected print file
    QMap<QString, QString> loadedPrintInfo; //Print file info
    QString loadedPrintFilepath; //Print file path
    QString currentUserID;

    QObject* root = engine.rootObjects().at(0); //Get the root object (in this case the Window)
    QQuickWindow* window = qobject_cast<QQuickWindow*>(root); //Cast to QWindow
    QIcon icon = QIcon("PCMakerspace3DPKiosk/resources/PC-Logo.ico");
    window->setIcon(icon); //Set window icon

    //Connect backend printLoaded signal (runs after parsed print file that was uploaded directly)
    QObject::connect(&bk, &QTBackend::printLoaded, root, [root, &loadedPrintFilepath, &loadedPrintInfo](const QString &gcodeFilepath, const QMap<QString, QString> &printInfo) {
        loadedPrintFilepath = gcodeFilepath; //set filepath
        loadedPrintInfo = printInfo; //set printinfo
        root->setProperty("appstate", AppState::Prep); //change QML appstate to show print Info
    });

    //Connect octoprint emulator jobLoaded signal (runs after print file recieved from OrcaSlicer)
    QObject::connect(&ope, &OctoprintEmulator::jobLoaded, root, [root, &loadedPrintFilepath, &loadedPrintInfo](const QString &filepath, const QMap<QString, QString> &printInfo) {
        loadedPrintFilepath = filepath; //set filepath
        loadedPrintInfo = printInfo; //set printinfo
        root->setProperty("appstate", AppState::Prep); //change QML appstate to show print info
    });

    //runs when RFID card is scanned successfully
    QObject::connect(&rfidReader, &LTx2A::cardScanned, root, [root, &bk, &currentUserID, &loadedPrintFilepath, &loadedPrintInfo, &pl]() { //Connect the rfidReader cardScanned event to the lambda
        if (rfidReader.hasNext()) { //If the cards scanned queue is not empty
            QString cardid = rfidReader.getNext().id.replace("\"", "").trimmed();
            if (root->property("appstate") == AppState::UserScan) {
                currentUserID = cardid;
                qDebug() << "User card scanned: " + cardid;

                //Get user info
                QSqlQuery userQuery;
                userQuery.prepare("SELECT cics, trainingCompleted, authLevel FROM users WHERE id = :id LIMIT 1");
                userQuery.bindValue(":id", cardid);
                userQuery.exec();
                if (!userQuery.next()) return bk.showMessage(root, "Your account is not Registered\nPlease Register at the Check-In Kiosk");
                bool isCICS = userQuery.value(0).toBool();
                bool training = userQuery.value(1).toBool();
                int authLevel = userQuery.value(2).toInt();

                //Calculate printDuration
                double printDuration = parseDuration(loadedPrintInfo["duration"]);

                //Print verification / authorization Logic
                if (authLevel < 1) { //0 is normal, 1 is staff, 2 is system admin, 3 is supervisor

                    //ensure all qualifications are met, show feedback message if not;
                    if (!isCICS) return bk.showMessage(root, "Sorry, but only CICS Students\nMay print at the Physical Computing Makerspace", "I Understand");
                    if (!training) return bk.showMessage(root, "Please ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);
                    qDebug() << printDuration;
                    if (printDuration > 6.0) return bk.showMessage(root, "Prints cannot be longer than 6 hours\nPlease split up your print and try again");
                }
            } else if (root->property("appstate") == AppState::StaffScan) {
                //Staff scan to confirm a user has completed 3D Printing training

                //Get staff info
                QSqlQuery userQuery;
                userQuery.prepare("SELECT authLevel FROM users WHERE id = :id LIMIT 1");
                userQuery.bindValue(":id", cardid);
                userQuery.exec();
                if (!userQuery.next()) return bk.showMessage(root, "This user is not Registered\nPlease ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);
                int authLevel = userQuery.value(0).toInt();

                //Check that the user is staff
                if (authLevel < 1) return bk.showMessage(root, "This user is not Staff\nPlease ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);

                //Update user so save their 3d printing status
                QSqlQuery updateUserQuery;
                updateUserQuery.prepare("UPDATE users SET trainingCompleted = 1 WHERE id = :id;");
                updateUserQuery.bindValue(":id", currentUserID);
                updateUserQuery.exec();
            } else return; //If we're not in one of the scan states, ignore the card scan

            //This code executes when a print is verified and authorized
            bk.showMessage(root, "Printing now!"); //show printing message

            //Update user print statistics
            QSqlQuery userQuery2; //Get current print stats
            userQuery2.prepare("SELECT printsStarted, filamentUsedGrams, printHours FROM users WHERE id = :id LIMIT 1");
            userQuery2.bindValue(":id", currentUserID);
            if (!userQuery2.exec() || !userQuery2.next()) {
                qCritical() << userQuery2.lastError().text();
            }

            //Increase stats as needed
            int printsStarted = userQuery2.value(0).toInt();
            double filamentUsed = userQuery2.value(1).toDouble();
            double printHours = userQuery2.value(2).toDouble();
            printsStarted++;
            filamentUsed+=loadedPrintInfo["weight"].toDouble();
            printHours+=parseDuration(loadedPrintInfo["duration"]);

            //Update stats to database
            QSqlQuery updateUserQuery2;
            updateUserQuery2.prepare("UPDATE users SET printsStarted = :ps, filamentUsedGrams = :fu, printHours = :ph WHERE id = :id;");
            updateUserQuery2.bindValue(":ps", printsStarted);
            updateUserQuery2.bindValue(":fu", filamentUsed);
            updateUserQuery2.bindValue(":ph", printHours);
            updateUserQuery2.bindValue(":id", currentUserID);
            updateUserQuery2.exec();
            if (!updateUserQuery2.exec()) {
                qCritical() << updateUserQuery2.lastError().text();
            }

            //Send print log to database
            QSqlQuery logPrintQuery;
            logPrintQuery.prepare("INSERT INTO printLog (durationHours, weight, printer, user, filament, filename, timestamp) "
                                  "VALUES(:dh, :wt, :pr, :us, :fm, :fn, :tm)");
            logPrintQuery.bindValue(":dh", parseDuration(loadedPrintInfo["duration"]));
            logPrintQuery.bindValue(":wt", loadedPrintInfo["weight"].toDouble());
            logPrintQuery.bindValue(":pr", loadedPrintInfo["printer"]);
            logPrintQuery.bindValue(":us", currentUserID);
            logPrintQuery.bindValue(":fm", loadedPrintInfo["filamentType"]);
            logPrintQuery.bindValue(":fn", loadedPrintInfo["filename"]);
            logPrintQuery.bindValue(":tm", QString("%1").arg(QDateTime::currentSecsSinceEpoch()));
            if (!logPrintQuery.exec()) {
                qCritical() << logPrintQuery.lastError().text();
            }

            //Send the print to the printer
            pl.startPrint(loadedPrintFilepath);
        }
    });

    QSqlQuery query2("DROP TABLE users"); //Demo stuff

    QSqlQuery query("CREATE TABLE IF NOT EXISTS users (" //Demo Stuff
                    "id VARCHAR(50) PRIMARY KEY, "
                    "firstName VARCHAR(50), "
                    "lastName VARCHAR(50), "
                    "email VARCHAR(80), "
                    "umass BOOLEAN, "
                    "cics BOOLEAN, "
                    "trainingCompleted BOOLEAN, "
                    "authLevel SMALLINT, "
                    "printsStarted INT, "
                    "filamentUsedGrams DOUBLE, "
                    "printHours DOUBLE)"
                );
    if (!query.isActive()) {
        qCritical() << "Failed to create table:" << query.lastError().text();
    }


    // QSqlQuery updateUserQuery;
    // updateUserQuery.prepare("UPDATE users SET authLevel = 2 WHERE id = :id;");
    // updateUserQuery.bindValue(":id", "09936544703440683676");
    // updateUserQuery.exec();
    //Demo Stuff
    QSqlQuery createUser;
    createUser.prepare("INSERT INTO users (id, firstName, lastName, email, umass, cics, trainingCompleted, authLevel, printsStarted, filamentUsedGrams, printHours) "
                         "VALUES ('09936544703440683676', 'Antony', 'Rinaldi', 'ajrinaldi@umass.edu', :um, :cs, :tc, :al, 0, 0.0, 0.0);");
    createUser.bindValue(":um", true);
    createUser.bindValue(":cs", true);
    createUser.bindValue(":tc", true);
    createUser.bindValue(":al", 0);
    if (!createUser.exec()) {
        qCritical() << "Failed to create user:" << createUser.lastError().text();
    }

    return app.exec(); //run the app
}
