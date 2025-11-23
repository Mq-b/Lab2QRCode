#include <QApplication>
#include "BarcodeWidget.h"
#include "logging.h"
#include "convert.h"

int main(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_Use96Dpi);
    QApplication app(argc, argv);
    Logging::setupLogging();
    BarcodeWidget w;
    w.show();
    return app.exec();
}
