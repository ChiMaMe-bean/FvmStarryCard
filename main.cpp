#include "stellarforge.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/items/icons/icon512.ico"));
    StellarForge w;
    w.show();
    return a.exec();
}
