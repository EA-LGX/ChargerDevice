#include "HttpDemo.h"
#include <QLabel>
#include <QSplitter>
#include <QTextEdit>
#include <QPushButton>

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

HttpDemo::HttpDemo(QWidget* parent)
    : QMainWindow(parent) {
    // 水平分割窗口
    QSplitter* splitter = new QSplitter(Qt::Horizontal, 0);
    QSplitter* splitter2 = new QSplitter(Qt::Vertical);
    splitter->addWidget(splitter2);
    QTextEdit* label = new QTextEdit("top Widget");
    label->setAlignment(Qt::AlignCenter);
    splitter2->addWidget(label);
    QTextEdit* label2 = new QTextEdit("center Widget");
    label2->setAlignment(Qt::AlignCenter);
    splitter2->addWidget(label2);
    QPushButton* btn = new QPushButton("发送请求");
    splitter2->addWidget(btn);

    QObject::connect(btn, &QPushButton::clicked, [=]() {
        getData("http://localhost:8084/user/findAll");
        });

    QTextEdit* label3 = new QTextEdit("right Widget");
    label3->setAlignment(Qt::AlignCenter);
    splitter->addWidget(label3);

    splitter->setWindowTitle("QSplitter");
    splitter->setGeometry(100, 100, 500, 300);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->show();
}

HttpDemo::~HttpDemo() {

}

void HttpDemo::getData(QString url) {
    // 通过Get请求获取数据
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    QNetworkRequest request;
    request.setUrl(QUrl(url));

    QNetworkReply* reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData.toUtf8());
            QJsonObject jsonObj = jsonDoc.object();
            QString name = jsonObj.value("userName").toString();
            QMessageBox::information(nullptr, "提示", jsonDoc.isArray() ? "是数组" : "不是数组");
        }
        else {
            QMessageBox::information(nullptr, "提示", "请求失败");
        }
        reply->deleteLater();
        });
}
