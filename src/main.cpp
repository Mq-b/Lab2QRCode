#include <QApplication>
#include "BarcodeWidget.h"
#include "logging.h"
#include "convert.h"

int main(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    Logging::setupLogging();
    BarcodeWidget w;
    w.show();
    return app.exec();
}
