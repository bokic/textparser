#include "dialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_ASSUME_STDERR_HAS_CONSOLE", "1");

    QApplication a(argc, argv);
    Dialog w;
    w.show();
    return a.exec();
}
