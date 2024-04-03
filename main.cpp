#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setApplicationName("TradingCatClient");
    QApplication::setOrganizationName("Cat software development");
    QApplication::setApplicationVersion(QString("Version:0.1 Build: %1 %2").arg(__DATE__).arg(__TIME__));

    MainWindow w;
    w.show();

    return a.exec();
}
