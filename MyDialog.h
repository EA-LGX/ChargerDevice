#include <QDialog>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QSpinBox>
#include <QObject>

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

class MyDialog : public QDialog {
public:
    MyDialog(QJsonObject jsonObj, QWidget* parent = nullptr) : QDialog(parent) {
        /**
            {
            "userAccount": "xingxing",
            "userPassword": "123456",
            "userName": "xingxingzi",
            "userAge": 21,
            "userSex": "男",
            "userCardId": "019A8926",
            "userCardBalance": 93.22,
            "userEmail": "",
            "userPhone": "17758540834"
            }
        */
        // 设置MyDialog字体12号
        QFont font("Microsoft YaHei", 12);
        this->setFont(font);
        QLabel* userName = new QLabel("用户名 :  " + jsonObj.value("userName").toString());
        QLabel* userCardId = new QLabel("卡号 :  " + jsonObj.value("userCardId").toString());

        // double 转 QString
        double balance = jsonObj.value("userCardBalance").toDouble();
        QString balanceStr = QString::number(balance, 'f', 2);
        QLabel* userCardBalance = new QLabel("余额 :  " + balanceStr + "元");
        //创建一个label 充电桩选择
        QLabel* label = new QLabel("充电桩选择");
        // 创建一个下拉框控件
        QComboBox* comboBox = new QComboBox(this);
        // 为comboBox设置14号字体
        comboBox->setFont(QFont("Microsoft YaHei", 14));
        comboBox->addItem("1号充电桩");
        comboBox->addItem("2号充电桩");
        comboBox->addItem("3号充电桩");
        comboBox->addItem("4号充电桩");
        QLabel* timeLabel = new QLabel("充电时间选择");
        // 添加一个SpinBox控件
        QSpinBox* spinBox = new QSpinBox(this);
        spinBox->setRange(1, 10);
        spinBox->setSuffix("小时");
        //字体
        spinBox->setFont(QFont("Microsoft YaHei", 14));
        QLabel* costLabel = new QLabel("充电费用 :  0.50元");
        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
            double cost = value * 0.5;
            QString costStr = QString::number(cost, 'f', 2);
            costLabel->setText("充电费用 :  " + costStr + "元");
            });
        QPushButton* button = new QPushButton("开始充电", this);

        // 创建一个垂直布局管理器并将下拉框添加到其中
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(userName);
        layout->addWidget(userCardId);
        layout->addWidget(userCardBalance);
        layout->addWidget(label);
        layout->addWidget(comboBox);
        layout->addWidget(timeLabel);
        layout->addWidget(spinBox);
        layout->addWidget(costLabel);
        layout->addWidget(button);

        //连接信号和槽
        connect(button, &QPushButton::clicked, this, [=]() {

            // 通过Post请求获取数据
            QNetworkAccessManager* manager = new QNetworkAccessManager();
            QNetworkRequest request;
            request.setUrl(QUrl("http://192.168.31.138:8084/cost/charge"));
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            QJsonObject json;

            json.insert("userCardId", jsonObj.value("userCardId").toString());
            json.insert("hour", spinBox->value());
            json.insert("cost", spinBox->value() * 0.5);
            json.insert("interfaceNum", comboBox->currentIndex() + 1);

            QJsonDocument jsonDoc(json);
            QByteArray postData = jsonDoc.toJson(QJsonDocument::Compact);

            QNetworkReply* reply = manager->post(request, postData);

            QObject::connect(reply, &QNetworkReply::finished, [=]() {
                if (reply->error() == QNetworkReply::NoError) {
                    QString responseData = reply->readAll();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData.toUtf8());
                    QJsonObject jsonObj = jsonDoc.object();
                    QString name = jsonObj.value("status").toString();
                    if (name == "success")
                        QMessageBox::information(nullptr, "提示", "充电成功");
                    else
                        QMessageBox::information(nullptr, "提示", "充电失败");
                }
                else {
                    QMessageBox::information(nullptr, "提示", "请求失败");
                    // m_console->putData(QString("请求失败"));
                }
                reply->deleteLater();
                });
            this->close();
            });

        // 设置对话框的标题和大小
        setWindowTitle("选择一个选项");
        resize(200, 100);
    }
};
