#include "headers/errorhandler.hpp"
#include <QDateTime>

Error Error::handle(QString t, QString m, ErrorLevel l) {
    Error _new = Error(t, m, l);
    if (_new.isError()) ErrorHandler::handle(_new);
    return _new;
};

void ErrorHandler::softHandle(const Error &err) {
    if (!err.isError()) return;
    printLn(err);
}

void ErrorHandler::handle(const Error &err) {
    if (!err.isError()) return;
    printLn(err);
    switch(err.level) {
    default:
    case El::None:
    case El::Debug:
        return;
    case El::Fatal:
        exit(1);
        break;
    case El::Trivial:
    case El::Warning:
    case El::Critical:
        if (err.type == "") {

        }
    }
}

QString ErrorHandler::genLogLine(const QString &lvl, const QString &content) {
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ss.zzzZ") + " " + lvl + " [ErrorHandler]: " + content;
}

void ErrorHandler::printLn(ErrorLevel lvl, const QString &content) {
    QString lvlindicator;
    switch (lvl) {
    default:
    case El::None:
        lvlindicator = "NONE";
        qDebug() << genLogLine(lvlindicator, content);
        break;
    case El::Debug:
        lvlindicator = "DEBUG";
        qDebug() << genLogLine(lvlindicator, content);
        break;
    case El::Trivial:
        lvlindicator = "TRIVIAL";
        qInfo() << genLogLine(lvlindicator, content);
        break;
    case El::Warning:
        lvlindicator = "WARNING";
        qWarning() << genLogLine(lvlindicator, content);
        break;
    case El::Critical:
        lvlindicator = "CRITICAL";
        qCritical() << genLogLine(lvlindicator, content);
        break;
    case El::Fatal:
        lvlindicator = "FATAL";
        qFatal() << genLogLine(lvlindicator, content);
        break;
    }
};

void ErrorHandler::printLn(const Error &err) {
    printLn(err.level, err.type + ": " + err.errorString);
}
