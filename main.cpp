#include "mainwindow.h"

#include <QApplication>
#include <QTextCodec>

int main(int argc, char* argv[]) {
    // 适配高分辨率屏幕
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);

    // 设置程序中文编码
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF8"));

    MainWindow w;
    w.show();
    return a.exec();
}
