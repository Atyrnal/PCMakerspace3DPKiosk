#include "headers/bambuemulator.h"

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QTcpServer>
#include <QNetworkDatagram>
#include "headers/errors.hpp"
#include <QTimer>

BambuEmulator::BambuEmulator(QObject* parent) : QObject(parent) {
    mosquito = new QProcess(parent);
    mqtt = new QMqttClient();

    QString mosquitoPath = "C:/Program Files/mosquitto/mosquitto.exe";
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("mosq.conf");

    mosquito->start(mosquitoPath, QStringList() << "-c" << configPath << "-v");
    if (!mosquito->waitForStarted(2000)) {
        Error("BambuEmulatorError", "Failed to start mosquito", El::Critical).handle();
        return;
    };

    QObject::connect(mosquito, &QProcess::readyReadStandardOutput, this, [this](){
        qDebug() << "[mosq]" << this->mosquito->readAllStandardOutput();
    });
    QObject::connect(mosquito, &QProcess::readyReadStandardError, this, [this](){
        qWarning() << "[mosq/err]" << this->mosquito->readAllStandardError();
    });
    QObject::connect(mosquito, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [](int code, QProcess::ExitStatus){ Error("BambuEmulatorError", "Mosquitto exited " + QString::number(code), El::Critical).handle(); });


    mqtt->setHostname("127.0.0.1");
    mqtt->setPort(8883);
    mqtt->setUsername("bblp");
    mqtt->setPassword("");
    mqtt->setClientId("PCM 3DP Kiosk Service");

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    mqtt->connectToHostEncrypted(sslConfig);

    QObject::connect(mqtt, &QMqttClient::connected, [](){
        qDebug() << "Connected to Mosquitto";
    });
    QObject::connect(mqtt, &QMqttClient::disconnected, [](){
        qDebug() << "Disconnected from Mosquitto";
    });

    QObject::connect(mqtt, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
        qDebug() << "Message from slicer on topic" << topic.name();
        if (BambuLab::requestFiler.match(topic)) {
            QJsonParseError err = QJsonParseError();
            QJsonDocument doc = QJsonDocument::fromJson(message, &err);
            if (err.error != QJsonParseError::NoError) {
                return Error("MqttJsonParseError", err.errorString(), El::Warning).handle();
            }
            if (!doc.isObject()) {
                return Error("MqttJsonParseError", "Message is not Json object", El::Warning).handle();
            }
            QJsonObject msg = doc.object();
            QString vSN = topic.name().split("/").at(1);
            if (printers.contains(vSN)) {
                BambuLab* printer = printers.value(vSN);
                if (msg.contains("print")) { //Handle print request somehwo

                } else { //Forward request to printer
                    printer->mqtt->publish(printer->requestTopic, QJsonDocument(msg).toJson(QJsonDocument::Compact));
                }
            }
        } else {

        }
    });
}

void BambuEmulator::slicerHandshake(QSslSocket* socket, const QString &vSN, BambuLab* printer) {
    QByteArray data = socket->readAll();

    if (data.size() < 6) return;
    if (data[0] != (char)0xa5 || data[1] != (char)0xa5) {
        Error("BambuEmulatorServerError", "Invalid packet header", El::Warning);
        return;
    }

    // Parse length (little-endian uint16)
    quint16 length = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(data.constData() + 2));

    // Extract JSON
    QByteArray jsonData = data.mid(4, length);
    qDebug() << "Received:" << jsonData;

    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) return Error("BambuEmulatorServerError", "Packet data is not Json Object", El::Warning).handle();

    QJsonObject msg = doc.object();
    if (!msg.contains("login")) return;

    QJsonObject login = msg["login"].toObject();
    QString command = login["command"].toString();
    QString seqId = login["sequence_id"].toString();

    if (command == "detect") {
        // Respond with printer info
        QJsonObject response;
        response["command"] = "detect";
        response["sequence_id"] = seqId;
        response["id"] = vSN;
        response["model"] = printer->modelId; //get from printer
        response["name"] = printer->name;
        response["version"] = printer->firmwareVer;
        response["bind"] = "free";
        response["connect"] = "lan";
        response["dev_cap"] = printer->devCap;

        QJsonObject wrapper;
        wrapper["login"] = response;

        //Send response
        QByteArray jsonData = QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
        quint16 length = jsonData.size();

        QByteArray packet;
        packet.append((char)0xa5);
        packet.append((char)0xa5);
        packet.append(reinterpret_cast<const char*>(&length), 2); // Little-endian
        packet.append(jsonData);
        packet.append((char)0xa7);
        packet.append((char)0xa7);

        socket->write(packet);
        socket->flush();

        qDebug() << "Sent response:" << jsonData;
    }
}

void BambuEmulator::addPrinter(quint32 id, BambuLab* printer) {
    if (printer->virtualSN != "undefined") {
        disconnect(printer);
        printers.insert(printer->virtualSN, printer);
        SNs.insert(id, printer->virtualSN);
        Error fperr = fetchPrinterInfo(printer);
        if (fperr.isError()) {
            return fperr.handle();
        }
        connect(printer, &BambuLab::messageRecieved, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
            //Forward messages from printer to slicer
            mqtt->publish(topic, message);
        });



        QString virtualIP = QString("127.0.0.%1").arg(id + 2);

        QTcpServer* tcpServer = new QTcpServer(this);
        if (!tcpServer->listen(QHostAddress(virtualIP), 3002)) {
            Error("BambuEmulatorServerError", "Failed to listen on" + virtualIP + ":3002" ,El::Critical).handle();
            return;
        }

        qDebug() << "Printer" << printer->name << "listening on" << virtualIP << ":3002";

        connect(tcpServer, &QTcpServer::newConnection, this, [this, tcpServer, printer]() {
            QTcpSocket* socket = tcpServer->nextPendingConnection();
            qDebug() << "Connection received for" << printer->name;

            // Wrap in SSL
            QSslSocket* sslSocket = new QSslSocket(this);
            if (!sslSocket->setSocketDescriptor(socket->socketDescriptor())) {
                Error("BambuEmulatorServerError", "Failed to set socket descriptor", El::Critical).handle();
                socket->deleteLater();
                sslSocket->deleteLater();
                return;
            }

            // Configure SSL
            sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
            sslSocket->setProtocol(QSsl::TlsV1_2OrLater);
            sslSocket->setLocalCertificate("server.crt");
            sslSocket->setPrivateKey("server.key");

            connect(sslSocket, &QSslSocket::encrypted, this, [sslSocket, printer]() {
                qDebug() << "SSL handshake complete for" << printer->name;
            });

            // connect(sslSocket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            //         [sslSocket](const QList<QSslError>& errors) {
            //             qWarning() << "SSL errors:" << errors;
            //             sslSocket->ignoreSslErrors();
            //         });

            connect(sslSocket, &QSslSocket::readyRead, this, [this, sslSocket, printer]() {
                slicerHandshake(sslSocket, printer->virtualSN, printer);
            });

            connect(sslSocket, &QSslSocket::disconnected, [sslSocket]() {
                qDebug() << "Client disconnected from port 3002";
                sslSocket->deleteLater();
            });

            // Start SSL handshake
            sslSocket->startServerEncryption();
        });
    } else { //Wait for VSN set signal, then add
        connect(printer, &BambuLab::setVSN, this, [this, &id, printer](const QString &vSN) {
            this->addPrinter(id, printer);
        });
    }
}

Error BambuEmulator::fetchPrinterInfo(BambuLab* printer) {
    QTcpSocket socket;
    socket.connectToHost(printer->hostname, 3002);

    if (!socket.waitForConnected(5000)) {
        return Error("BambuEmulatorFetchError", "Failed to connect to printer", El::Warning);
    }

    // Wrap in SSL
    QSslSocket sslSocket;
    sslSocket.setSocketDescriptor(socket.socketDescriptor());
    sslSocket.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslSocket.setProtocol(QSsl::TlsV1_2OrLater);

    sslSocket.startClientEncryption();
    if (!sslSocket.waitForEncrypted(5000)) {
        return Error("BambuEmulatorFetchError", "SSL handshake failed", El::Warning);
    }

    //qDebug() << "Connected to" << printer->hostname << "via SSL";

    // Create detect command
    QJsonObject login;
    login["command"] = "detect";
    login["sequence_id"] = "50000";

    QJsonObject wrapper;
    wrapper["login"] = login;

    QByteArray jsonData = QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
    quint16 length = jsonData.size();

    QByteArray packet;
    packet.append((char)0xa5);
    packet.append((char)0xa5);
    packet.append(reinterpret_cast<const char*>(&length), 2);
    packet.append(jsonData);
    packet.append((char)0xa7);
    packet.append((char)0xa7);

    sslSocket.write(packet);
    sslSocket.flush();

    //qDebug() << "Sent detect command";

    // Wait for response
    if (!sslSocket.waitForReadyRead(5000)) {
        return Error("BambuEmulatorFetchError", "No response from printer", El::Warning);
    }

    QByteArray response = sslSocket.readAll();
    //qDebug() << "Received" << response.size() << "bytes";

    if (response.size() < 6) {
        return Error("BambuEmulatorFetchError", "Response too short", El::Warning);
    }

    if (response[0] != (char)0xa5 || response[1] != (char)0xa5) {
        return Error("BambuEmulatorFetchError", "Invalid response header", El::Warning);
    }

    quint16 respLength = qFromLittleEndian<quint16>(reinterpret_cast<const uchar*>(response.constData() + 2));
    QByteArray jsonResponse = response.mid(4, respLength);

    qDebug() << "Response JSON:" << jsonResponse;

    QJsonDocument doc = QJsonDocument::fromJson(jsonResponse);
    if (!doc.isObject()) {
        return Error("BambuEmulatorFetchError", "Response is not JSON", El::Warning);
    }

    QJsonObject respObj = doc.object();
    if (!respObj.contains("login")) {
        return Error("BambuEmulatorFetchError", "Response malformed", El::Warning);
    }

    QJsonObject loginResp = respObj["login"].toObject();

    //info.id = loginResp["id"].toString();
    printer->modelId = loginResp["model"].toString();
    //info.name = loginResp["name"].toString();
    printer->firmwareVer = loginResp["version"].toString();
    //info.bind = loginResp["bind"].toString();
    //info.connect = loginResp["connect"].toString();
    printer->devCap = loginResp["dev_cap"].toInt();
    //info.valid = true;

    sslSocket.disconnectFromHost();
    sslSocket.deleteLater();
    return Error::None();
}

void BambuEmulator::removePrinter(quint32 id) {
    if (!SNs.contains(id)) return;
    QString sn = SNs.value(id);
    disconnect(printers.value(sn));
    printers.remove(sn);
    SNs.remove(id);
}

void BambuEmulator::closing() {
    mosquito->kill();
}

BambuEmulator::~BambuEmulator() {
    mosquito->kill();
}
