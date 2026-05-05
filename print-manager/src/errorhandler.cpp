#include "errorhandler.hpp"
#include <QDateTime>

Error Error::handle(QString t, QString m, ErrorLevel l) {
    Error _new = Error(t, m, l);
    if (_new.isError()) ErrorHandler::handle(_new);
    return _new;
};

Error Error::softHandle(QString t, QString m, ErrorLevel l) {
    Error _new = Error(t, m, l);
    if (_new.isError()) ErrorHandler::softHandle(_new);
    return _new;
};

void Error::handle() const {
    if (this->isError()) ErrorHandler::handle(*this);
};

void Error::softHandle() const {
    if (this->isError()) ErrorHandler::softHandle(*this);
};

void ErrorHandler::initLogFile(const QString &path) {
    logFile = new QFile(path);
    if (logFile->open(QIODevice::Append | QIODevice::Text)) {
        logStream = new QTextStream(logFile);
    } else {
        delete logFile;
        logFile = nullptr;
    }
}

void ErrorHandler::writeToFile(const QString &line) {
    if (logStream == nullptr) return;
    *logStream << line << "\n";
    logStream->flush();
}

void ErrorHandler::softHandle(const Error &err) {
    if (!err.isError()) return;
    printLn(err);
}

void Log::write() const {
    ErrorHandler::log(*this);
}

class Log Log::write(QString t, QString m) {
    Log _new = Log(t, m);
    ErrorHandler::log(_new);
    return _new;
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

void ErrorHandler::log(const class Log &log) {
    QString line = genLogLineLog(log.type, log.message);
    writeToFile(line);
    qDebug().noquote().nospace() << line << "\033[0m";
}


QString ErrorHandler::genLogLine(const QString &lvl, const QString &content) {
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ss.zzzZ") + " " + lvl + " [ErrorHandler]: " + content;
}

QString ErrorHandler::genLogLineLog(const QString &type, const QString &content) {
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ss.zzzZ") + " " + "LOG  " + " ["+type+"]: " + content;
}

void ErrorHandler::printLn(ErrorLevel lvl, const QString &content) {
    QString lvlindicator;
    QString line;
    switch (lvl) {
    default:
    case El::None:
        lvlindicator = "NONE ";
        line = genLogLine(lvlindicator, content);
        writeToFile(line);
        qDebug().noquote().nospace() << line  << "\033[0m";
        break;
    case El::Log:
        break;
    case El::Debug:
        lvlindicator = "DEBUG";
        line = genLogLine(lvlindicator, content);
        writeToFile(line);
        qDebug().noquote().nospace() << line << "\033[0m";
        break;
    case El::Trivial:
        lvlindicator = "TRIV ";
        line = genLogLine(lvlindicator, content);
        writeToFile(line);
        qInfo().noquote().nospace() << line << "\033[0m";
        break;
    case El::Warning:
        lvlindicator = "WARN ";
        line = genLogLine(lvlindicator, content);
        writeToFile(line);
        qWarning().noquote().nospace() << "\033[33m" << line << "\033[0m";
        break;
    case El::Critical:
        lvlindicator = "CRIT ";
        line = genLogLine(lvlindicator, content);
        writeToFile(line);
        qCritical().noquote().nospace() << "\033[31m" << line << "\033[0m";
        break;
    case El::Fatal:
        lvlindicator = "FATAL";
        line = genLogLine(lvlindicator, content);
        writeToFile(line);
        qFatal().noquote().nospace() << "\033[41m" << line << "\033[0m";
        break;
    }
};

void ErrorHandler::printLn(const Error &err) {
    printLn(err.level, err.type + ": " + err.errorString);
}
