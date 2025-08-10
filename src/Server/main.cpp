#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QThreadPool>

#include "ExampleServer.hpp"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
    qDebug(QStringLiteral("main thread: QThread(0x%1)").arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16, QLatin1Char('0')).toStdString().c_str());
    QDir::setCurrent(qApp->applicationDirPath());

    QCommandLineParser cmdParser;
    cmdParser.addHelpOption();
    cmdParser.addPositionalArgument("host", "Server listen address.");
    cmdParser.addPositionalArgument("port", "Server listen port.");
    cmdParser.process(a);

    const QStringList posArgs = cmdParser.positionalArguments();
    if (posArgs.size() < 2)
    {
        cmdParser.showHelp();
        return 1;
    }

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());

    QTimer::singleShot(0, [posArgs](){
        Net::ConnectionSettings serverSettings;
        serverSettings.ipLocal.setAddress(posArgs.at(0));
        serverSettings.portIn = posArgs.at(1).toUInt();
        ExampleServer* server = new ExampleServer(serverSettings);
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [server](){
            delete server;
        });
    });

    return a.exec();
}
