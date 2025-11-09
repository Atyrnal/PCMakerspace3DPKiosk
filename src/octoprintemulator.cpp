/*
 *
 * Copyright (c) 2025 Antony Rinaldi
 *
*/


#include "headers/octoprintemulator.h"
#include <QJsonObject>
#include <QTcpServer>
#include <QHttpServerRequest>
#include <QFile>
#include <QDir>
#include <QHttpServerResponse>
#include "headers/gcodeparser.h"

OctoprintEmulator::OctoprintEmulator(QObject* parent) : QObject(parent), server() {
    server.route("/api/printer", []() {
        qDebug() << "/api/printer called!";
        return QJsonObject {
            {"state", QJsonObject{{"text", "Operational"}}},
            {"temperature", QJsonObject{{"tool0", QJsonObject{{"actual", 200}, {"target", 210}}}}}
        };
    });

    server.route("/api/server", []() {
        qDebug() << "/api/server called!";
        return QJsonObject {
            {"version", "1.5.0"},
            {"safemode", "incomplete_startup"}
        };
    });

    server.route("/api/version", []() {
        qDebug() << "/api/version called!";
        return QJsonObject {
            {"api", "0.1"},
            {"version", "1.3.10"},
            {"text", "OctoPrint 1.3.10"}
        };
    });

    server.route("/api/job", this, [this](const QHttpServerRequest &request) {
        qDebug() << "/api/job called!";
        if (!request.headers().contains("Content-Type") || request.headers().value("Content-Type") != "application/json") return QHttpServerResponse("Expected Content-Type application/json", QHttpServerResponder::StatusCode::BadRequest);
        QJsonParseError error;
        QJsonObject obj;
        QJsonDocument doc = QJsonDocument::fromJson(request.body(), &error);
        if (error.error == QJsonParseError::NoError) {
            if (doc.isObject()) {
                obj = doc.object();
            }
        } else {
            qWarning() << "Failed to parse JSON:" << error.errorString();
            return QHttpServerResponse("Failed to parse json", QHttpServerResponder::StatusCode::InternalServerError);
        }
        if (!obj.keys().contains("command")) return QHttpServerResponse("Expected command", QHttpServerResponder::StatusCode::BadRequest);
        if (obj.value("command") != "start") return QHttpServerResponse(QHttpServerResponder::StatusCode::NotImplemented);
        QMap<QString, QString> properties = GCodeParser::parseFile(fileInfo->absoluteFilePath());
        QVariantMap propertiesForJS;
        for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
            propertiesForJS.insert(it.key(), it.value());
        }
        emit jobInfoLoaded(propertiesForJS);
        emit jobLoaded(fileInfo->absoluteFilePath(), properties);
        return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
    });


    server.route("/api/files/<arg>", this, [this](const QString &location, const QHttpServerRequest &request) -> QHttpServerResponse {
        qDebug() << "/api/files called!";

        if (!request.headers().contains("Content-Type") || !request.headers().value("Content-Type").contains("multipart/form-data")) {
            return QHttpServerResponse("Expected multipart/form-data", QHttpServerResponder::StatusCode::BadRequest);
        }

        QByteArray body = request.body();

        // Extract boundary from Content-Type
        QString contentType = QString(request.headers().value("Content-Type").toByteArray());
        QString boundary;
        static QRegularExpression re("boundary=(.+)");
        QRegularExpressionMatch match = re.match(contentType);
        if (match.hasMatch()) {
            boundary = "--" + match.captured(1);
        } else {
            return QHttpServerResponse("No boundary found", QHttpServerResponder::StatusCode::BadRequest);
        }

        //QList<QByteArray> parts = body.split('\n');

        // convert to string for easier processing
        QString bodyStr = QString::fromUtf8(body);

        //find filename
        static QRegularExpression fileNameRe(R"delim(filename="([^"]+)")delim");
        QRegularExpressionMatch fileNameMatch = fileNameRe.match(bodyStr);
        QString originalFileName = fileNameMatch.hasMatch() ? fileNameMatch.captured(1) : "uploaded.gcode";
        QString filePath = "uploaded/" + originalFileName;

        QList<QString> multipartPartsStr = bodyStr.split(boundary);
        QList<QByteArray> multipartParts;
        for (auto it = multipartPartsStr.constBegin(); it != multipartPartsStr.constEnd(); ++it) {
            multipartParts.append(it->toUtf8());
        }

        // Variables to hold file and other fields
        QByteArray fileData;
        bool selectFlag = false;
        bool printFlag = false;

        for (const QByteArray &partRaw : multipartParts) {
            if (partRaw.trimmed().isEmpty()) continue;

            QString part = QString::fromUtf8(partRaw);
            // Check if this is the file part
            if (part.contains("name=\"file\"")) {
                // Extract file data: after empty line
                int index = part.indexOf("\r\n\r\n");
                if (index < 0) index = part.indexOf("\n\n");
                if (index >= 0) {
                    fileData = partRaw.mid(index + 4); // skip headers
                    // remove any trailing boundary markers
                    int boundaryIndex = fileData.indexOf("\r\n--");
                    if (boundaryIndex >= 0) fileData.truncate(boundaryIndex);
                }
            }

            // Check select/print fields
            if (part.contains("name=\"select\"")) {
                selectFlag = part.contains("true");
            }
            if (part.contains("name=\"print\"")) {
                printFlag = part.contains("true");
            }
        }
        QDir uploadDir = QDir("uploaded");
        if (!uploadDir.exists()) {
            uploadDir.mkpath(".");
        }
        QFileInfoList oldfiles = uploadDir.entryInfoList(QDir::Files);
        for (int i = 0; i < oldfiles.size(); ++i) {
            const QFileInfo &fileInfo = oldfiles.at(i);
            if (!uploadDir.remove(fileInfo.fileName())) {
                qWarning() << "Failed to remove file:" << fileInfo.fileName();
            }
        }
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            return QHttpServerResponse("Failed to write file", QHttpServerResponder::StatusCode::InternalServerError);
        }
        file.write(fileData);
        file.close();
        this->fileInfo = new QFileInfo(filePath);

        // Parse the gcode properties
        QMap<QString, QString> properties = GCodeParser::parseFile(fileInfo->absoluteFilePath());
        properties.insert("filename", originalFileName);
        QVariantMap propertiesForJS;
        for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
            propertiesForJS.insert(it.key(), it.value());
        }

        emit jobInfoLoaded(propertiesForJS);
        emit jobLoaded(fileInfo->absoluteFilePath(), properties);

        // Build JSON response
        QJsonObject localFile{
            {"name", originalFileName},
            {"path", filePath},
            {"type", "machinecode"},
            {"origin", location},
            {"refs", QJsonObject{
                         {"resource", QString("http://localhost:5000/api/files/%1/%2").arg(location, filePath)},
                         {"download", QString("http://localhost:5000/downloads/files/%1/%2").arg(location, filePath)}
                     }}
        };
        QJsonObject files{{location, localFile}};
        QJsonObject response{
            {"files", files},
            {"done", true},
            {"effectiveSelect", true},
            {"effectivePrint", false}
        };

        return QHttpServerResponse(response, QHttpServerResponder::StatusCode::Created);
    });

    /*server.route("/api/files/<arg>", this, [this](const QString &location, const QHttpServerRequest &request) -> QHttpServerResponse {
        qDebug() << "/api/files called!";
        if (!request.headers().contains("Content-Type") || !request.headers().value("Content-Type").contains("multipart/form-data")) {
            return QHttpServerResponse("Expected multipart/form-data", QHttpServerResponder::StatusCode::BadRequest);
        }

        QByteArray body = request.body();
        QString filename = "uploaded.gcode";
        QFile file(filename);
        if (!file.open(QIODevice::WriteOnly)) {
            return QHttpServerResponse("Failed to write file", QHttpServerResponder::StatusCode::InternalServerError);
        }
        file.write(body);
        file.close();
        this->fileInfo = QFileInfo(filename);

        QMap<QString, QString> properties = GCodeParser::parseFile(fileInfo.absoluteFilePath());
        QVariantMap propertiesForJS;
        for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
            propertiesForJS.insert(it.key(), it.value());
        }
        emit jobInfoLoaded(propertiesForJS);
        emit jobLoaded(fileInfo.absoluteFilePath(), properties);

        QJsonObject localFile{
            {"name", filename},
            {"path", filename},
            {"type", "machinecode"},
            {"origin", location},
            {"refs", QJsonObject{
                         {"resource", QString("http://localhost:5000/api/files/%1/%2").arg(location, filename)},
                         {"download", QString("http://localhost:5000/downloads/files/%1/%2").arg(location, filename)}
                     }}
        };
        QJsonObject files{{location, localFile}};
        QJsonObject response{
            {"files", files},
            {"done", true},
            {"effectiveSelect", true},
            {"effectivePrint", false}
        };

        return QHttpServerResponse(response);
    });*/

    /*server.route("<arg>", [](const QString &everything, const QHttpServerRequest &request) -> QHttpServerResponse {
        qDebug() << "Endpoint: " << everything;
        qDebug() << "Headers:" << request.headers();
        qDebug() << "Body size:" << request.body().size();

        return QHttpServerResponse(QHttpServerResponder::StatusCode::Ok);
    });*/


    quint16 port = 5000; // whatever port you want
    QTcpServer* tcp = new QTcpServer(this); // create TCP server
    if (!tcp->listen(QHostAddress::LocalHost, 5000)) { // start listening
        qCritical() << "Failed to start TCP server on port 5000";
        return;
    }

    if (!server.bind(tcp)) { // bind QHttpServer to TCP server
        qCritical() << "Failed to bind QHttpServer to TCP server";
        return;
    }
}
