#include "main_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/images/push.png"));
    MainWindow w;
    w.show();
    return a.exec();
}
