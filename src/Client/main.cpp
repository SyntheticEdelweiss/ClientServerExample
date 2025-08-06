#include <QtCore/QDir>
#include <QtWidgets/QApplication>

#include "MainWindow.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDir::setCurrent(qApp->applicationDirPath());

    MainWindow* w = new MainWindow();
    w->show();

    return a.exec();
}
