#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "MyDialog.h"

#include <QLabel>
#include <QMessageBox>
#include <QtCore/QDateTime>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QTimer>

#include <QDebug>
#include <QDataStream>
#include <QIODevice>

#include <QTextCodec>

#define port1 "130"
#define port2 "132"
#define port3 "133"
#define port4 "134"

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_settings(new SettingsDialog),
    m_serial(new QSerialPort(this)) {
    m_ui->setupUi(this);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    setBeepState(0);
    initActionsConnections();
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serial, SIGNAL(readyRead()), this, SLOT(readData()));
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(handletimeout()));
    getCurrentAvailabel();
    // 每3秒获取一次
    QTimer* timer1 = new QTimer(this);
    connect(timer1, &QTimer::timeout, [=]() {
        getCurrentAvailabel();
        });
    timer1->start(3000); // 每隔3秒钟执行一次getCurrentAvailabel()函数

    // 初始化MQTT
    m_client = new QMqttClient(this);
    m_client->setHostname(QString("43.138.179.149"));
    m_client->setPort(1883);
    m_client->setClientId(QString("SuperChargerTerminal"));
    m_client->setUsername(QString("admin"));
    m_client->setPassword(QString("whiteone@lu"));
    // 订阅主题
    m_client->connectToHost();
    // 连接成功
    connect(m_client, &QMqttClient::connected, this, [=]() {
        auto subscription = m_client->subscribe(QString("ToDevice/1001"));
        });

    connect(m_client, &QMqttClient::messageReceived, this, [this](const QByteArray& message, const QMqttTopicName& topic) {
        const QString content = QDateTime::currentDateTime().toString()
            + QLatin1String(" Received Topic: ")
            + topic.name()
            + QLatin1String(" Message: ")
            + message
            + QLatin1Char('\n');

        setBeepState(1);
        QTimer::singleShot(100, this, [=]() {
            setBeepState(0);
            });

        // message转为QString
        QString messageStr = QString::fromUtf8(message);

        //message={"port":1,"chargeHour":1,"userCardId":"019A8926"}
        //将message转为json对象
        QJsonDocument jsonDoc = QJsonDocument::fromJson(messageStr.toUtf8());
        QJsonObject jsonObj = jsonDoc.object();
        int port = jsonObj.value("port").toInt();
        int chargeHour = jsonObj.value("chargeHour").toInt();
        QString userCardId = jsonObj.value("userCardId").toString();

        // 通过Post请求获取数据
        QNetworkAccessManager* manager = new QNetworkAccessManager();
        QNetworkRequest request;
        request.setUrl(QUrl("http://192.168.31.138:8084/cost/charge"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject json;

        json.insert("userCardId", userCardId);
        json.insert("hour", chargeHour);
        json.insert("cost", chargeHour * 0.5);
        json.insert("interfaceNum", port);

        QJsonDocument jsonDoc1(json);
        QByteArray postData = jsonDoc1.toJson(QJsonDocument::Compact);

        QNetworkReply* reply = manager->post(request, postData);

        QObject::connect(reply, &QNetworkReply::finished, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QString responseData = reply->readAll();
                QJsonDocument jsonDoc2 = QJsonDocument::fromJson(responseData.toUtf8());
                QJsonObject jsonObj = jsonDoc2.object();
                QString name = jsonObj.value("status").toString();
                if (name == "success")
                    QMessageBox::information(nullptr, "提示", "充电成功");
                else
                    QMessageBox::information(nullptr, "提示", "充电失败");
            }
            else {
                QMessageBox::information(nullptr, "提示", "请求失败");
            }
            reply->deleteLater();
            });
        });
}

void MainWindow::handletimeout() {
    this->sendTest();
}

MainWindow::~MainWindow() {
    delete m_settings;
    delete m_ui;
}

void MainWindow::openSerialPort() {
    const SettingsDialog::Settings p = m_settings->settings();
    m_serial->setPortName(p.name);
    m_serial->setBaudRate(p.baudRate);
    m_serial->setDataBits(p.dataBits);
    m_serial->setParity(p.parity);
    m_serial->setStopBits(p.stopBits);
    m_serial->setFlowControl(p.flowControl);
    if (m_serial->open(QIODevice::ReadWrite)) {
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
            .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
            .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    }
    else {
        QMessageBox::critical(this, tr("Error"), m_serial->errorString());
        showStatusMessage(tr("Open error"));
    }
}

void MainWindow::closeSerialPort() {
    if (m_serial->isOpen())
        m_serial->close();
    // m_console->setEnabled(false);
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::about() {
    QMessageBox::about(this, tr("About Simple Terminal"),
        tr("The <b>Simple Terminal</b> example demonstrates how to "
            "use the Qt Serial Port module in modern GUI applications "
            "using Qt, with a menu bar, toolbars, and a status bar."));
}

void MainWindow::writeData(const QByteArray& data) {
    m_serial->write(data);
}

void MainWindow::readData() {
    const QByteArray data = m_serial->readAll();
    // 暂停接收
    m_serial->close();

    //接收中文的时候要设置对应的编码
    //比如在Windows上用串口发送数据，Windows上面编码是gb2312，接收的时候也要用gb2312去编码接收数据，不然就会乱码
    QString str = QTextCodec::codecForName("GB2312")->toUnicode(data);
    // QString str = QTextCodec::codecForName("UTF-8")->toUnicode(data);
    if (str.contains("ID")) {
        // str="ID: AAAAAAAA";截取卡号从ID开始数起
        str = str.mid(str.indexOf("ID") + 4, 8);
    }
    setBeepState(1);
    QTimer::singleShot(100, this, [=]() {
        setBeepState(0);
        });
    getUserInfo(str);
}

void MainWindow::handleError(QSerialPort::SerialPortError error) {
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
        closeSerialPort();
    }
}

void MainWindow::initActionsConnections() {
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    //connect(m_ui->actionClear, &QAction::triggered, m_console, &Console::clear);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    connect(m_ui->actionSend, &QAction::triggered, this, &MainWindow::sendTest);
    connect(m_ui->actionRead, &QAction::triggered, this, &MainWindow::readTest);
    connect(m_ui->actionTimer, &QAction::triggered, this, &MainWindow::timerTest);

}

void MainWindow::showStatusMessage(const QString& message) {
    // m_status->setText(message);
}

void MainWindow::sendTest() {
    this->writeData("https://firebbs.cn/\r\n");
}

void MainWindow::readTest() {
    QByteArray data = m_serial->readAll();
    this->writeData(data);
}

void MainWindow::timerTest() {
    static bool timerState = false;
    timerState = !timerState;
    if (timerState) {
        if (this->m_serial->isOpen()) {
            this->timer->start(1000);
            m_ui->actionTimer->setText("stop timer");
        }
    }
    else {
        this->timer->stop();
        m_ui->actionTimer->setText("start timer");
    }
}

void MainWindow::getUserInfo(QString cardID) {
    // 通过Post请求获取数据
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("http://192.168.31.138:8084/user/findUserByUserCardId"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json.insert("userCardId", cardID);
    QJsonDocument jsonDoc(json);
    QByteArray postData = jsonDoc.toJson(QJsonDocument::Compact);

    QNetworkReply* reply = manager->post(request, postData);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString responseData = reply->readAll();
            if (!responseData.contains("userAccount")) {
                QString sendBody = "{\"userCardId\":\"" + cardID + "\"}";
                QMessageBox::information(nullptr, "提示", "还没有绑定卡,打开手机进行绑定");
                if (m_client->publish(QString("toAndroidCardBinding/1001"), sendBody.toUtf8()) == -1)
                    QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not publish message"));
            }
            else {
                QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData.toUtf8());
                QJsonObject jsonObj = jsonDoc.object();
                MyDialog dialog(jsonObj);
                dialog.exec();
            }
        }
        else {
            QMessageBox::information(nullptr, "提示", "请求失败");
            // m_console->putData(QString("请求失败"));
        }
        reply->deleteLater();
        // 继续接收
        m_serial->open(QIODevice::ReadWrite);
        });
}

void MainWindow::getCurrentAvailabel() {
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("http://192.168.31.138:8084/charger/findAllWhereUsing"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    QJsonDocument jsonDoc(json);
    QByteArray postData = jsonDoc.toJson(QJsonDocument::Compact);
    QNetworkReply* reply = manager->post(request, postData);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData.toUtf8());
            QJsonObject jsonObj = jsonDoc.object();
            // QString data = jsonObj.value("data").toString();
            // QMessageBox::information(nullptr, "提示", data);
            if (responseData.contains("port1")) {
                m_ui->label_port1->setText("1号剩余充电时长: " + QString(jsonObj.value("port1").toString()) + "分钟");
                setLEDState(port1, 1);
            }
            else {
                m_ui->label_port1->setText("1号: 空闲");
                setLEDState(port1, 0);
            }
            if (responseData.contains("port2")) {
                m_ui->label_port2->setText("2号剩余充电时长: " + QString(jsonObj.value("port2").toString()) + "分钟");
                setLEDState(port2, 1);
            }
            else {
                m_ui->label_port2->setText("2号: 空闲");
                setLEDState(port2, 0);
            }
            if (responseData.contains("port3")) {
                m_ui->label_port3->setText("3号剩余充电时长: " + QString(jsonObj.value("port3").toString()) + "分钟");
                setLEDState(port3, 1);
            }
            else {
                m_ui->label_port3->setText("3号: 空闲");
                setLEDState(port3, 0);
            }
            if (responseData.contains("port4")) {
                m_ui->label_port4->setText("4号剩余充电时长: " + QString(jsonObj.value("port4").toString()) + "分钟");
                setLEDState(port4, 1);
            }
            else {
                m_ui->label_port4->setText("4号: 空闲");
                setLEDState(port4, 0);
            }
        }
        else {
            QMessageBox::information(nullptr, "提示", "请求失败");
            // m_console->putData(QString("请求失败"));
        }
        reply->deleteLater();
        // 继续接收
        m_serial->open(QIODevice::ReadWrite);
        });
}

void MainWindow::setBeepState(int status) {
#ifdef __arm__
    QString strFile = "/sys/class/gpio/gpio19/value";
    QFile file(strFile);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qDebug() << "/sys/class/gpio/gpio19 export failed!";
        return;
    }
    file.write(1 == status ? "1" : "0");
    file.close();
#else
    qDebug() << "state" << status;
#endif
}

void MainWindow::setLEDState(QString index, int status) {
#ifdef __arm__
    QString strFile = "/sys/class/gpio/gpio" + index + "/value";
    QFile file(strFile);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qDebug() << strFile + " export failed!";
        return;
    }
    file.write(1 == status ? "1" : "0");
    file.close();
#else
    qDebug() << "state:" << status;
#endif
}
