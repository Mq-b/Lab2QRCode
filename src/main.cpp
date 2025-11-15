#include "BarcodeWidget.h"
#include <QApplication>
#include "logging.h"
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Logging::initSpdlog();
    BarcodeWidget w;
    w.show();
    return app.exec();
}