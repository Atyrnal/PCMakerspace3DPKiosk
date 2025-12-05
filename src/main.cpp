/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QSqlError>
#include <QSqlQuery>
#include "headers/ltx2aQT.h"
#include <QQmlContext>
#include "headers/qtbackend.h"
#include <QQuickWindow>
#include <QFile>
#include <QDir>

//Atyrnal 10/29/2025

//See ui/Main.qml for ui declarations

LTx2A rfidReader;

QJsonObject readJsonFile(const QString &filepath, quint64 maxSize = 0) {
    QFile f = QFile(filepath);
    if (!f.exists()) return {{"_error", "file not found: " + filepath}};
    if (!f.open(QFile::ReadOnly)) return {{"_error", "unable to open file: " + filepath}};
    if (maxSize > 0 && f.size() > maxSize) return {{"_error", "file size " + QString::number(f.size()) + " larger than maximum " + QString::number(maxSize)}};
    QByteArray data = f.read(f.size());
    f.close();
    QJsonParseError p;
    QJsonDocument doc = QJsonDocument::fromJson(data, &p);
    if (p.error != QJsonParseError::NoError) return {{"_error", "JSON parse error: " + p.errorString()}};

    if (!doc.isObject()) return {{"_error", "file is not JSON Object"}};

    QJsonObject obj = doc.object();
    obj.insert("_error", "false");

    return obj;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv); //Create a QT gui app

    rfidReader.start(); //Initialize the RFID reader
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &rfidReader, &LTx2A::stop); //Connect the aboutToQuit app event to the rfidReader's stop function

    QQmlApplicationEngine engine; //Qt engine creation
    QObject::connect( //Exit if object creation fails
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    QTBackend bk(&engine);
    engine.loadFromModule("PCMakerspace3DPKiosk", "Main"); //Load the QML Main.qml declarative ui file

    QObject* root = engine.rootObjects().at(0); //Get the root object (in this case the Window)
    bk.setRoot(root);


    QJsonObject config = readJsonFile(QDir(QCoreApplication::applicationDirPath()).filePath("debug_configuration.json"), 1000000);
    if (config.value("_error").toString("should never happen") != "false") {
        qCritical() << config.value("_error").toString("Error: Parsed JSON object missing error status specifier");
    } else {
        bk.loadConfig(config);
    }


    QQuickWindow* window = qobject_cast<QQuickWindow*>(root); //Cast to QWindow
    QIcon icon = QIcon("PCMakerspace3DPKiosk/resources/PC-Logo.ico");
    window->setIcon(icon); //Set window icon

    //runs when RFID card is scanned successfully
    QObject::connect(&rfidReader, &LTx2A::cardScanned, &bk, [&bk]() { //Connect the rfidReader cardScanned event to the lambda
        if (rfidReader.hasNext()) { //If the cards scanned queue is not empty
            QString cardid = rfidReader.getNext().id.replace("\"", "").trimmed();
            bk.cardScanned(cardid);
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
    createUser.bindValue(":al", 2);
    if (!createUser.exec()) {
        qCritical() << "Failed to create user:" << createUser.lastError().text();
    }

    //Create second user for demo
    QSqlQuery createUser2;
    createUser2.prepare("INSERT INTO users (id, firstName, lastName, email, umass, cics, trainingCompleted, authLevel, printsStarted, filamentUsedGrams, printHours) "
                       "VALUES ('', 'Judge', 'Judy', 'judge@judy.com', :um, :cs, :tc, :al, 0, 0.0, 0.0);");
    createUser2.bindValue(":um", true);
    createUser2.bindValue(":cs", true);
    createUser2.bindValue(":tc", false);
    createUser2.bindValue(":al", 0);
    if (!createUser2.exec()) {
        qCritical() << "Failed to create user:" << createUser2.lastError().text();
    }

    auto result = bk.queryDatabase("SELECT firstName FROM users WHERE id = :id LIMIT 1", QMap<QString, QVariant>{{":id", "09936544703440683676"}});
    qDebug() << result;

    return app.exec(); //run the app
}
