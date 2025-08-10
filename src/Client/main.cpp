#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtWidgets/QApplication>

#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(qApp->applicationDirPath());

    QCommandLineParser cmdParser;
    cmdParser.addHelpOption();
    cmdParser.addPositionalArgument("username", "username to authorize with.");
    cmdParser.addPositionalArgument("password", "password to authorize with.");
    cmdParser.process(a);

    const QStringList posArgs = cmdParser.positionalArguments();
    if (posArgs.size() < 2)
    {
        cmdParser.showHelp();
        return 1;
    }

    QTimer::singleShot(0, [posArgs](){
        MainWindow* w = new MainWindow(Net::LoginData{posArgs.at(0), posArgs.at(1)});
        w->show();
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [w](){
            delete w;
        });
    });

    return a.exec();
}
