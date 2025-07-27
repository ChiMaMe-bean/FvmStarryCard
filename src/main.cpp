#include "core/starrycard.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/icon512.ico"));
    StarryCard w;
    w.show();
    return a.exec();
}
