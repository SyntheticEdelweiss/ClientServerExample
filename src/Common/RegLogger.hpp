#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtCore/QTimer>

class RegLoggerThreadWorker;

class RegLogger : public QObject
{
    Q_OBJECT
    friend class RegLoggerThreadWorker;
private:
    ~RegLogger();
    RegLogger();
    RegLogger(const RegLogger&)             = delete; // Copy constructor
    RegLogger(RegLogger&&)                  = delete; // Move constructor
    RegLogger& operator=(const RegLogger&)  = delete; // Copy assignment
    RegLogger& operator=(RegLogger&&)       = delete; // Move assignment

private:
    QHash<uint, QFile*> m_fileMap;
    uint m_fileIdCounter = 0;

    QString m_registryDirPath;

    const QString m_dateTimeFormat = "[yyyy.MM.dd-hh:mm:ss:zzz]";
    const QString m_timeFormat = "[hh:mm:ss:zzz]";
    const QString m_dateFormat = "yyyy-MM-dd";
    const QString m_fileExtension = ".log";

public:
    static RegLogger& instance();

private slots:
    void remakeDir();

public slots:
    uint onAddFile(QString fileName);
    void onRemoveFile(uint fileId);
    void onLogData(uint fileId, QString data, qint64 timestamp);

signals:
    uint addFile(QString fileName); // NOTE: this function uses BLOCKING queued connection since it needs to return id of newly registered file
    void removeFile(uint fileId);
    void logData(uint fileId, QString data, qint64 timestamp = QDateTime::currentMSecsSinceEpoch());
};

class RegLoggerThreadWorker : public QObject
{
    Q_OBJECT
    friend class RegLogger;
private:
    RegLoggerThreadWorker(QString a_registryDirPath);
    ~RegLoggerThreadWorker();
    RegLoggerThreadWorker(const RegLoggerThreadWorker&)             = delete; // Copy constructor
    RegLoggerThreadWorker(RegLoggerThreadWorker&&)                  = delete; // Move constructor
    RegLoggerThreadWorker& operator=(const RegLoggerThreadWorker&)  = delete; // Copy assignment
    RegLoggerThreadWorker& operator=(RegLoggerThreadWorker&&)       = delete; // Move assignment

    static bool isRegLoggerInstantiated;
    const QString m_registryDirPath;

public:
    static void instantiateRegLogger(QString a_registryDirPath); // Must be called after main QEventLoop is started, otherwise will cause endless loop (due to unprocessed signal)
    static void deleteRegLogger(); // Must be called BEFORE main QEventLoop is finished, otherwise will cause endless loop (due to unprocessed signal)

private slots:
    void process();

signals:
    void created();
};
