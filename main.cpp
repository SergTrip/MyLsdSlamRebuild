#include "mainliveodometry.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainLiveOdometry w;
    w.show();

    return a.exec();
}
