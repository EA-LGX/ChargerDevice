#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"
#include "MyDialog.h"

#include <QLabel>
#include <QMessageBox>
#include <QTimer>

#include <QDebug>
#include <QDataStream>
#include <QIODevice>

#include <QTextCodec>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    // m_status(new QLabel),
    // m_console(new Console),
    m_settings(new SettingsDialog),
    m_serial(new QSerialPort(this)) {
    m_ui->setupUi(this);
    // m_console->setEnabled(false);
    // setCentralWidget(m_console);

    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    // m_ui->statusBar->addWidget(m_status);
    initActionsConnections();
    connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serial, SIGNAL(readyRead()), this, SLOT(readData()));
    //connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    // connect(m_console, &Console::getData, this, &MainWindow::writeData);
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(handletimeout()));
    getCurrentAvailabel();
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
        // m_console->setEnabled(true);
        // m_console->setLocalEchoEnabled(p.localEchoEnabled);
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
            .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
            .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
        //timer->start(1000);
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
        // m_console->putData(str);
    }
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
    // m_console->putData(data);
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
    request.setUrl(QUrl("http://localhost:8084/user/findUserByUserCardId"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject json;
    json.insert("userCardId", cardID);
    QJsonDocument jsonDoc(json);
    QByteArray postData = jsonDoc.toJson(QJsonDocument::Compact);

    QNetworkReply* reply = manager->post(request, postData);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData.toUtf8());
            QJsonObject jsonObj = jsonDoc.object();
            QString name = jsonObj.value("userName").toString();
            // QMessageBox::information(nullptr, "提示", responseData);
            // m_console->putData("\n" + responseData);
            MyDialog dialog(jsonObj);
            dialog.exec();

        }
        else {
            QMessageBox::information(nullptr, "提示", "请求失败");
            // m_console->putData(QString("请求失败"));
        }
        reply->deleteLater();
        // 继续接收
        m_serial->open(QIODevice::ReadWrite);
        getCurrentAvailabel();
        });
}

void MainWindow::getCurrentAvailabel() {
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl("http://localhost:8084/charger/findAllWhereUsing"));
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

                m_ui->label_port1->setText("1号剩余充电时长: " + QString(jsonObj.value("port1").toInt()) + "分钟");
            }
            else {

            }
            if (responseData.contains("port2")) {

                m_ui->label_port2->setText("2号剩余充电时长: " + QString(jsonObj.value("port2").toInt()) + "分钟");
            }
            else {

            }
            if (responseData.contains("port3")) {

                m_ui->label_port3->setText("3号剩余充电时长: " + QString(jsonObj.value("port3").toInt()) + "分钟");
            }
            else {

            }
            if (responseData.contains("port4")) {

                m_ui->label_port4->setText("4号剩余充电时长: " + QString(jsonObj.value("port4").toInt()) + "分钟");
            }
            else {

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

