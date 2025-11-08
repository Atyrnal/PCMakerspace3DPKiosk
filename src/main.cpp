#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include "headers/ltx2aQT.h"
#include <QTimer>
#include <QQmlContext>
#include "headers/secrets.h"
#include "headers/qtbackend.h"

//Atyrnal 10/29/2025

//See ui/Main.qml for ui declarations

LTx2A rfidReader;

enum AppState {
    Idle,
    Message,
    Scan,
    Printing
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv); //Create a QT gui app

    app.setWindowIcon(QIcon("resources/PC-Logo.ico")); //Set the app Icon (not working)
    rfidReader.start(); //Initialize the RFID reader
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &rfidReader, &LTx2A::stop); //Connect the aboutToQuit app event to the rfidReader's stop function

    QQmlApplicationEngine engine; //Qt engine creation
    QObject::connect( //Exit if object creation fails
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("PCMakerspace3DPKiosk", "Main"); //Load the QML Main.qml declarative ui file



    QObject* root = engine.rootObjects().at(0); //Get the root object (in this case the Window)
    QTBackend bk;
    engine.rootContext()->setContextProperty("backend", &bk);

    QObject::connect(&rfidReader, &LTx2A::cardScanned, root, [root]() { //Connect the rfidReader cardScanned event to the lambda
        if (rfidReader.hasNext()) { //If the cards scanned queue is not empty
            qDebug() << "Card scanned" << rfidReader.getNext().id;
        }
    });

    QTimer::singleShot(5000, root, [root](){
        root->setProperty("appstate", AppState::Scan);
    });


    return app.exec(); //run the app
}
