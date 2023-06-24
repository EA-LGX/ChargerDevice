#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

// Json 解析
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QMessageBox>

// 网络请求
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QEventLoop>
#include <QMqttClient>

QT_BEGIN_NAMESPACE

class QLabel;

namespace Ui {
    class MainWindow;
}

QT_END_NAMESPACE

class Console;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void about();
    void writeData(const QByteArray& data);
    void readData();

    void handleError(QSerialPort::SerialPortError error);

    void handletimeout();

    void sendTest();
    void readTest();
    void timerTest();

private:
    void initActionsConnections();
    void setBeepState(int status);          // 设置蜂鸣器状态
    void setLEDState(QString index,int status);

private:
    void showStatusMessage(const QString& message);
    void getUserInfo(QString cardID);   // 网络请求
    void getCurrentAvailabel();         // 网络请求

    Ui::MainWindow* m_ui = nullptr;
    QLabel* m_status = nullptr;
    // Console* m_console = nullptr;
    SettingsDialog* m_settings = nullptr;
    QSerialPort* m_serial = nullptr;

    QTimer* timer;
    QTimer* timer1;
    QMqttClient* m_client;
};

#endif // MAINWINDOW_H
