#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("MineSweeper"));
    app.setApplicationDisplayName(QStringLiteral("扫雷"));

    MainWindow w;
    w.show();
    return app.exec();
}
