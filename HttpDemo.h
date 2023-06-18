#ifndef HTTPDEMO_H
#define HTTPDEMO_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class HttpDemo; }
QT_END_NAMESPACE

class HttpDemo : public QMainWindow {
    Q_OBJECT

public:
    HttpDemo(QWidget* parent = nullptr);
    ~HttpDemo();

private:
    void getData(QString url);
};

#endif