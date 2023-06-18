#include "mainwindow.h"
#include "MyDialog.h"
#include <QApplication>
#include <QTextCodec>

int main(int argc, char* argv[]) {
    // 适配高分辨率屏幕
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);

    // 设置程序中文编码
    QTextCodec* codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    MainWindow w;
    w.show();

    // MyDialog dialog("12345678");
    return a.exec();
}
