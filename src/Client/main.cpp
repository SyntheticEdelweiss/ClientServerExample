#include <QtCore/QDir>
#include <QtWidgets/QApplication>

#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(qApp->applicationDirPath());

    QTimer::singleShot(0, [](){
        MainWindow* w = new MainWindow();
        w->show();
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [w](){
            delete w;
        });
    });

    return a.exec();
}
