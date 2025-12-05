#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QString>
#include <QDebug>
#include <QObject>
#include <QVariant>

enum ErrorLevel {
    None,
    Debug,
    Trivial,
    Warning,
    Critical,
    Fatal
};

class Error {
public:
    Error(QString t = "None", QString m= "", ErrorLevel l = ErrorLevel::Trivial) : errorString(m), type(t), level(l) {};
    const QString errorString;
    const QString type;
    const ErrorLevel level;
    static Error None() { return Error("None", "", ErrorLevel::None); }
    static Error handle(QString t = "None", QString m= "", ErrorLevel l = ErrorLevel::Trivial);
    bool isError() const {
        return level == ErrorLevel::None || type == "None";
    }
    friend QDebug operator<<(QDebug debug, const Error &err) {
        QDebugStateSaver saver(debug);
        debug.nospace() << err.type << "(\"" << err.errorString << "\", " << err.level <<")";
        return debug;
    };
};

template<typename T>
class ErrorOption {
public:
    ErrorOption(Error* er) {
        errorObj = er;
        isErr = true;
    };
    ErrorOption(T value) {
        this->value = value;
        errorObj = nullptr;
        isErr = false;
    }
    ErrorOption(T value, Error* error) {
        this->value = value;
        errorObj = error;
        isErr = true;
    };
    ErrorOption(T value, QString errorType, QString errorMsg, ErrorLevel errorLvl = ErrorLevel :: Debug) {
        this->value = value;
        errorObj = new Error(errorType, errorMsg, errorLvl);
        isErr = true;
    };
    ErrorOption(QString errorType, QString errorMsg, ErrorLevel errorLvl = ErrorLevel :: Trivial) {
        errorObj = new Error(errorType, errorMsg, errorLvl);
        isErr = true;
    }
    T get() const {
        return value;
    };
    T getOrDefault(T def) const {
        return (isError()) ? def : value;
    };
    T getOr(T def) const {
        return getOrDefault(def);
    };
    QString errorType() const {
        if (errorObj == nullptr) return "None";
        return errorObj->type;
    }
    QString errorString() const {
        if (errorObj == nullptr) return "";
        return errorObj->errorString;
    };
    ErrorLevel errorLevel() const {
        if (errorObj == nullptr) return ErrorLevel::None;
        return errorObj->level;
    }
    Error* error() const {
        return errorObj;
    };
    bool isError() const {
        return isErr;
    };
    friend QDebug operator<<(QDebug debug, const ErrorOption &err) {
        QDebugStateSaver saver(debug);
        debug.nospace() << "ErrorOption(" << ((err.errorObj->isError()) ? Error::None() : *err.errorObj) << ", " << err.value <<")";
        return debug;
    };
private:
    T value;
    Error* errorObj;
    bool isErr;
};


template<typename T>
using Eo = ErrorOption<T>;

using El = ErrorLevel;

#endif // ERRORHANDLER_H
