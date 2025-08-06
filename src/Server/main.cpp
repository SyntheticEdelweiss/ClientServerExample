#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

#include "ExampleServer.hpp"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
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

    Net::ConnectionSettings serverSettings;
    serverSettings.ipLocal.setAddress(posArgs.at(0));
    serverSettings.portIn = posArgs.at(1).toUInt();
    ExampleServer server(serverSettings);
    Q_UNUSED(server);

    return a.exec();
}
