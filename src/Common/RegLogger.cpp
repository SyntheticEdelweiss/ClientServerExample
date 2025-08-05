#include "RegLogger.hpp"

bool RegLoggerThreadWorker::isRegLoggerInstantiated = false;

RegLogger::~RegLogger()
{
    for (auto iter = m_fileMap.begin(); iter != m_fileMap.end(); ++iter)
    {
        iter.value()->close();
    }
    qDeleteAll(m_fileMap);
    m_fileMap.clear();
}

RegLogger::RegLogger()
{
    connect(this, &RegLogger::addFile,    this, &RegLogger::onAddFile,    Qt::BlockingQueuedConnection);
    connect(this, &RegLogger::removeFile, this, &RegLogger::onRemoveFile, Qt::QueuedConnection);
    connect(this, &RegLogger::logData,    this, &RegLogger::onLogData,    Qt::QueuedConnection);
}

RegLogger& RegLogger::instance()
{
    static RegLogger* pLoggerInstance = new RegLogger;
    return *pLoggerInstance;
}

void RegLogger::remakeDir()
{
    QDir currentDir(m_registryDirPath);
    currentDir.mkpath(m_registryDirPath);
}

uint RegLogger::onAddFile(QString fileName)
{
    for (auto iter = m_fileMap.begin(); iter != m_fileMap.end(); ++iter)
    {
        if (fileName == QFileInfo(*(iter.value())).fileName())
            return iter.key();
    }

    uint fileId = ++m_fileIdCounter;    
    QString fileFullName = QString("%1/%2_%3%4").arg(m_registryDirPath, fileName, QDateTime::currentDateTimeUtc().date().toString(m_dateFormat), m_fileExtension);
    QFile* pFile = new QFile(fileFullName);
    if (!pFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        delete pFile;
        return 0;
    }
    m_fileMap.insert(fileId, pFile);
    return fileId;
}

void RegLogger::onRemoveFile(uint fileId)
{
    auto fileIter = m_fileMap.find(fileId);
    if (fileIter == m_fileMap.end())
        return;

    delete fileIter.value();
    m_fileMap.erase(fileIter);
    return;
}

void RegLogger::onLogData(uint fileId, QString data, qint64 timestamp)
{
    auto fileIter = m_fileMap.find(fileId);
    if (fileIter == m_fileMap.end())
        return;

    QTextStream stream(fileIter.value());
    stream << QDateTime::fromMSecsSinceEpoch(timestamp).time().toString(m_timeFormat) << ' ' << data << '\n';
    fileIter.value()->flush();
    return;
}

// <--------------------------------- RegLoggerThreadWorker -------------------------------->

RegLoggerThreadWorker::RegLoggerThreadWorker(QString a_registryDirPath)
    : m_registryDirPath(a_registryDirPath)
{}

RegLoggerThreadWorker::~RegLoggerThreadWorker()
{
    isRegLoggerInstantiated = false;
}

void RegLoggerThreadWorker::instantiateRegLogger(QString a_registryDirPath)
{
    if (isRegLoggerInstantiated)
        return;

    // Instantiate RegLogger in separate thread
    QThread* pLoggerThread = new QThread;
    RegLoggerThreadWorker* pLoggerWorker = new RegLoggerThreadWorker(QDir(a_registryDirPath).absolutePath());
    pLoggerWorker->moveToThread(pLoggerThread);
    QObject::connect(pLoggerThread, &QThread::started, pLoggerWorker, &RegLoggerThreadWorker::process);
    // connects for &RegLogger::destroyed are done in RegLoggerThreadWorker::process since they require instance of RegLogger
    QObject::connect(pLoggerThread, &QThread::finished, pLoggerThread, &QThread::deleteLater);

    bool isCreated = false;
    connect(pLoggerWorker, &RegLoggerThreadWorker::created, pLoggerWorker, [&isCreated](){ isCreated = true; });
    pLoggerThread->start();
    while (!isCreated)
        QThread::msleep(5);

    isRegLoggerInstantiated = true;
    return;
}

void RegLoggerThreadWorker::deleteRegLogger()
{
    if (!isRegLoggerInstantiated)
        return;

    QThread* pLoggerThread = RegLogger::instance().thread();
    RegLogger::instance().deleteLater();
    while (pLoggerThread->isFinished() == false) // According to docs, deleteLater() will perform deletion in the thread that created it, that is main thread, so calling it like this should be safe
        QThread::currentThread()->msleep(5);

    return;
}

void RegLoggerThreadWorker::process()
{
    RegLogger& regLogger = RegLogger::instance();
    regLogger.m_registryDirPath = this->m_registryDirPath;
    regLogger.remakeDir();
    QObject::connect(&regLogger, &QObject::destroyed, this, &QObject::deleteLater);
    // OBLIGATORY need to specify DirectConnection, since RegLogger is associated with QThread, which is associated with main thread, so AutoConnection leads to QueuedConnection
    // and that means QThread::quit won't be called until QCoreApplication::processEvents() is called in main thread, which leads to endless loop in deleteRegLogger()
    QObject::connect(&regLogger, &QObject::destroyed, regLogger.thread(), &QThread::quit, Qt::DirectConnection);
    emit created();
    return;
}
