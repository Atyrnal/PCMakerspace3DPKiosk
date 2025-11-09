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

double parseDuration(QString durationString) {
    static QRegularExpression regex(R"((?:(\d+(?:\.\d+)?)h)?\s*(?:(\d+(?:\.\d+)?)m)?\s*(?:(\d+(?:\.\d+)?)s)?)");
    QRegularExpressionMatch match = regex.match(durationString);

    if (!match.hasMatch()) return 999.99;
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

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("kioskdb.db");

    if (!db.open()) {
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
    QTBackend bk;
    PrusaLink pl;
    OctoprintEmulator ope;
    engine.rootContext()->setContextProperty("backend", &bk);
    engine.rootContext()->setContextProperty("octoprintemulator", &ope);
    engine.loadFromModule("PCMakerspace3DPKiosk", "Main"); //Load the QML Main.qml declarative ui file

    QFile* loadedPrint = nullptr;
    QMap<QString, QString> loadedPrintInfo;
    QString loadedPrintFilepath;
    QString currentUserID;

    QObject* root = engine.rootObjects().at(0); //Get the root object (in this case the Window)
    QQuickWindow* window = qobject_cast<QQuickWindow*>(root);
    QIcon icon = QIcon("PCMakerspace3DPKiosk/resources/PC-Logo.ico");
    window->setIcon(icon);
    QObject::connect(&bk, &QTBackend::printLoaded, root, [root, &loadedPrintFilepath, &loadedPrintInfo](const QString &gcodeFilepath, const QMap<QString, QString> &printInfo) {
        loadedPrintFilepath = gcodeFilepath; //this could be a problem
        loadedPrintInfo = printInfo;
        root->setProperty("appstate", AppState::Prep);
    });

    QObject::connect(&ope, &OctoprintEmulator::jobLoaded, root, [root, &loadedPrintFilepath, &loadedPrintInfo](const QString &filepath, const QMap<QString, QString> &printInfo) {
        loadedPrintFilepath = filepath; //this could be a problem
        loadedPrintInfo = printInfo;
        root->setProperty("appstate", AppState::Prep);
        /*if (root->visibility() == QWindow::Minimized)
        root->showNormal();
        root->raise();
        root->requestActivate();*/
    });

    QObject::connect(&rfidReader, &LTx2A::cardScanned, root, [root, &bk, &currentUserID, &loadedPrintFilepath, &loadedPrintInfo, &pl]() { //Connect the rfidReader cardScanned event to the lambda
        if (rfidReader.hasNext()) { //If the cards scanned queue is not empty
            QString cardid = rfidReader.getNext().id.replace("\"", "").trimmed();
            if (root->property("appstate") == AppState::UserScan) {
                currentUserID = cardid;
                qDebug() << "User card scanned: " + cardid;
                QSqlQuery userQuery;
                userQuery.prepare("SELECT cics, trainingCompleted, authLevel FROM users WHERE id = :id LIMIT 1");
                userQuery.bindValue(":id", cardid);
                userQuery.exec();
                if (!userQuery.next()) return bk.showMessage(root, "Your account is not Registered\nPlease Register at the Check-In Kiosk");
                bool isCICS = userQuery.value(0).toBool();
                bool training = userQuery.value(1).toBool();
                int authLevel = userQuery.value(2).toInt();
                double printDuration = parseDuration(loadedPrintInfo["duration"]);
                if (authLevel < 1) { //0 is normal, 1 is staff, 2 is system admin, 3 is supervisor
                    if (!isCICS) return bk.showMessage(root, "Sorry, but only CICS Students\nMay print at the Physical Computing Makerspace", "I Understand");
                    if (!training) return bk.showMessage(root, "Please ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);
                    qDebug() << printDuration;
                    if (printDuration > 6.0) return bk.showMessage(root, "Prints cannot be longer than 6 hours\nPlease split up your print and try again");
                }
            } else if (root->property("appstate") == AppState::StaffScan) {
                QSqlQuery userQuery;
                userQuery.prepare("SELECT authLevel FROM users WHERE id = :id LIMIT 1");
                userQuery.bindValue(":id", cardid);
                userQuery.exec();
                if (!userQuery.next()) return bk.showMessage(root, "This user is not Registered\nPlease ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);
                int authLevel = userQuery.value(0).toInt();
                if (authLevel < 1) return bk.showMessage(root, "This user is not Staff\nPlease ask a staff member for\nour 3D print training and have them\nscan their UCard to continue", "Training Completed", AppState::StaffScan);
                QSqlQuery updateUserQuery;
                updateUserQuery.prepare("UPDATE users SET trainingCompleted = 1 WHERE id = :id;");
                updateUserQuery.bindValue(":id", currentUserID);
                updateUserQuery.exec();
            } else return;
            bk.showMessage(root, "Printing now!");
            QSqlQuery userQuery2;
            userQuery2.prepare("SELECT printsStarted, filamentUsedGrams, printHours FROM users WHERE id = :id LIMIT 1");
            userQuery2.bindValue(":id", currentUserID);
            if (!userQuery2.exec() || !userQuery2.next()) {
                qCritical() << userQuery2.lastError().text();
            }
            int printsStarted = userQuery2.value(0).toInt();
            double filamentUsed = userQuery2.value(1).toDouble();
            double printHours = userQuery2.value(2).toDouble();
            printsStarted++;
            filamentUsed+=loadedPrintInfo["weight"].toDouble();
            printHours+=parseDuration(loadedPrintInfo["duration"]);
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
            pl.startPrint(loadedPrintFilepath);
        }
    });

    //QSqlQuery query2("DROP TABLE users");

    QSqlQuery query("CREATE TABLE IF NOT EXISTS users ("
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


    QSqlQuery updateUserQuery;
    updateUserQuery.prepare("UPDATE users SET authLevel = 2 WHERE id = :id;");
    updateUserQuery.bindValue(":id", "09936544703440683676");
    updateUserQuery.exec();
    /*QSqlQuery createUser("INSERT INTO users (id, firstName, lastName, email, umass, cics, trainingCompleted, authLevel, printsStarted, filamentUsedGrams, printHours) "
                         "VALUES ('08050260079957443132', 'Greg', 'Greg', 'Greg', 1, 1, 1, 1, 0, 0.0, 0.0);");
    if (!createUser.isActive()) {
        qCritical() << "Failed to create user:" << createUser.lastError().text();
    }*/

    return app.exec(); //run the app
}
