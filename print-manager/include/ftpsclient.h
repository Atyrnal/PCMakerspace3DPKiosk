// ftpsclient.h
#ifndef FTPSCLIENT_H
#define FTPSCLIENT_H

#include <QObject>
#include <QThread>
#include <curl/curl.h>
#include <QFile>

// Worker lives on the thread — no parent, moved via moveToThread
class FtpsWorker : public QObject {
    Q_OBJECT
public:
    explicit FtpsWorker() : QObject(nullptr) {}
public slots:
    void doUpload(const QString &localFile, const QString &host,
                  const QString &username, const QString &password,
                  const QString &remotePath);
signals:
    void progress(qint64 bytesSent, qint64 totalBytes);
    void finished(bool success, const QString &errorString);
private:
    static size_t readCallback(void *ptr, size_t size, size_t nmemb, void *stream);
    static int progressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                                curl_off_t ultotal, curl_off_t ulnow);
    CURL* m_curl = nullptr;
};

class FtpsClient : public QObject {
    Q_OBJECT
public:
    explicit FtpsClient(QObject* parent = nullptr);
    ~FtpsClient();
    void uploadFile(const QString &localFile, const QString &host,
                    const QString &username, const QString &password,
                    const QString &remotePath);
signals:
    void progress(qint64 bytesSent, qint64 totalBytes);
    void finished(bool success, const QString &errorString);
    // Internal signal to invoke worker across thread boundary
    void startUpload(const QString &localFile, const QString &host,
                     const QString &username, const QString &password,
                     const QString &remotePath);
private:
    QThread* m_thread;
    FtpsWorker* m_worker;
};

#endif // FTPSCLIENT_H