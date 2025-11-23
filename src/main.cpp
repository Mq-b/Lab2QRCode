#include <QApplication>
#include "BarcodeWidget.h"
#include "logging.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    Logging::setupLogging();
    BarcodeWidget w;
    w.show();
    return app.exec();
}
