/***************************************************************************
    Layout: A Track Editor for OutRun
    - Program Entry Point

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/


#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    
    // Pass control to QT and enter the event loop
    return app.exec();
}
