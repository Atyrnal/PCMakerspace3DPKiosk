#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include "headers/ltx2aQT.h"
#include <QTimer>
#include "headers/qrmanager.h"
#include <QQmlContext>
#include "headers/secrets.h"

//Atyrnal 10/29/2025

//See ui/Main.qml for ui declarations

LTx2A rfidReader;

enum AppState {
    Idle,
    Register,
    Welcome
};

void updateQR(QObject* root, QRManager* manager) { //Handles updating the qr
    manager->updateQRCode(QString(FORMURL) + rfidReader.getNext().id.trimmed()); //Re-generate the QR Image and draw it
    root->setProperty("appstate", AppState::Register); //Trigger ui state transition from Idle to QR
    QTimer::singleShot(16000, root, [root, manager]() { //Wait 12000ms, then
        if (rfidReader.hasNext()) { //If another card has been scanned
            updateQR(root, manager); //Update the QR again
        } else {
            root->setProperty("appstate", AppState::Idle); //Otherwise transition uo state back to Idles
        }
    });
}

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
    QRImageProvider* provider = new QRImageProvider; //Initialize qr image provider (dynamically provides images to the qml ui)
    provider->setData("https://atyrnal.dev"); //Inital qr code before any cards are scanned
    engine.addImageProvider("qr", provider); //Register the image provider so it can be accessed by QML
    QRManager* manager = new QRManager(provider); //Create the qr manager (interacts with ui and qr provider to handle updating the qr code)
    engine.rootContext()->setContextProperty("QRManager", manager); //Expose the manager functions to QML
    engine.loadFromModule("PCMakerspaceCheckin", "Main"); //Load the QML Main.qml declarative ui file



    QObject* root = engine.rootObjects().at(0); //Get the root object (in this case the Window)

    QObject::connect(&rfidReader, &LTx2A::cardScanned, root, [root, manager]() { //Connect the rfidReader cardScanned event to the lambda
        if (rfidReader.hasNext()) { //If the cards scanned queue is not empty
            updateQR(root, manager); //In the future change this to handle airtable information and welcome people
        }
    });

    return app.exec(); //run the app
}
