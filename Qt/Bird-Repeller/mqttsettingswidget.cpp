#include "mqttsettingswidget.h"

MqttSettingsWidget::MqttSettingsWidget(QWidget *parent) : QWidget(parent)
{
    mqttConfigFileName = "config/mqtt_config.json";
    mqttJsonObject["mqttServerAddr"] = "139.9.223.99";
    mqttJsonObject["mqttServerPort"] = 1883;
    mqttJsonObject["mqttUsername"] = "Qt";
    mqttJsonObject["mqttPassword"] = "123456";
    /* 打开文件，如果文件不存在就创建 */
    if (!readJsonFromFile(mqttConfigFileName, mqttJsonObject)) {
        writeJsonToFile(mqttConfigFileName, mqttJsonObject);
    }

    /* 设置窗口标题 */
    setWindowTitle("设置");
//    this->resize(700, 450);        /* 设置窗口大小 */
    this->setMinimumSize(700, 450);
    this->setMaximumSize(700, 450);
    /* 设置控件字体和大小 */
    QFont windowFont("得意黑", 16);
    setFont(windowFont);
    /* 标签 */
    serverAddrLabel = new QLabel("MQTT服务器IP地址：", this);
    serverPortLabel = new QLabel("MQTT服务器端口号：", this);
    usernameLabel = new QLabel("MQTT登录用户账号：", this);
    passwordLabel = new QLabel("MQTT登录用户密码：", this);

    serverAddrLabel->setGeometry(50, 50, 300, 50);
    serverPortLabel->setGeometry(50, 100, 300, 50);
    usernameLabel->setGeometry(50, 200, 300, 50);
    passwordLabel->setGeometry(50, 250, 300, 50);

    /* 输入框 */
    serverAddrLineEdit = new QLineEdit(getServerAddr(), this);
    serverPortLineEdit = new QLineEdit(QString::number(getServerPort(), 10), this);
    usernameLineEdit = new QLineEdit(getUsername(), this);
    passwordLineEdit = new QLineEdit(getPassword(), this);

    serverAddrLineEdit->setGeometry(350, 50, 300, 50);
    serverPortLineEdit->setGeometry(350, 100, 300, 50);
    usernameLineEdit->setGeometry(350, 200, 300, 50);
    passwordLineEdit->setGeometry(350, 250, 300, 50);

    /* 保存和退出按键 */
    saveButton = new QPushButton("保存", this);
    quitButton = new QPushButton("退出", this);
    saveButton->setGeometry(150, 350, 150, 80);
    quitButton->setGeometry(450, 350, 150, 80);
    connect(saveButton, &QPushButton::clicked, this, &MqttSettingsWidget::on_saveButton_chicked);
    connect(quitButton, &QPushButton::clicked, this, &MqttSettingsWidget::on_quitButton_chicked);
}

MqttSettingsWidget::~MqttSettingsWidget()
{
}

QString MqttSettingsWidget::getServerAddr()
{
    if (!readJsonFromFile(mqttConfigFileName, mqttJsonObject)) {
        return "";
    }
    return mqttJsonObject["mqttServerAddr"].toString();
}

int MqttSettingsWidget::getServerPort()
{
    if (!readJsonFromFile(mqttConfigFileName, mqttJsonObject)) {
        return 0;
    }
    QJsonValue portValue = mqttJsonObject["mqttServerPort"];
    if (portValue.isDouble() | portValue.isString()) {
        int port = portValue.toInt();
        return port;
    } else {
        qDebug() << "mqttServerPort 不是有效的数字！";
        return 0;
    }
}

QString MqttSettingsWidget::getUsername()
{
    if (!readJsonFromFile(mqttConfigFileName, mqttJsonObject)) {
        return "";
    }
    return mqttJsonObject["mqttUsername"].toString();
}

QString MqttSettingsWidget::getPassword()
{
    if (!readJsonFromFile(mqttConfigFileName, mqttJsonObject)) {
        return "";
    }
    return mqttJsonObject["mqttPassword"].toString();
}

bool MqttSettingsWidget::readJsonFromFile(const QString &fileName, QJsonObject &jsonObject)
{
    QString folderPath = "config";  // 文件夹路径
    QDir dir;
    // 检查文件夹是否存在
    if (!dir.exists(folderPath)) {
        // 如果文件夹不存在，尝试创建文件夹
        if (dir.mkpath(folderPath)) {
            qDebug() << "文件夹创建成功: " << folderPath;
        } else {
        }
    }
    QFile file(fileName);
    if (!file.exists()) {
        qDebug() << "文件不存在!";
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件:" << file.errorString();
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    // 尝试解析 JSON 数据
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) {
        qDebug() << "JSON 数据无效或不是对象!";
        return false;
    }

    jsonObject = doc.object();
    return true;
}

void MqttSettingsWidget::writeJsonToFile(const QString &fileName, QJsonObject &jsonObject)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件写入默认数据:" << file.errorString();
        return;
    }
    QJsonDocument defaultJsonDoc(jsonObject);
    file.write(defaultJsonDoc.toJson());
    file.close();

    qDebug() << "写入数据成功!";
}

void MqttSettingsWidget::on_saveButton_chicked()
{
    QString portText = serverPortLineEdit->text().trimmed();
    bool transitionFlag;
//    int port = serverPortLineEdit->text().simplified().toInt();
    int port = portText.toInt(&transitionFlag);;
    if (!transitionFlag) {
        QMessageBox::warning(NULL, "警告", "请输入正确的端口号");
        return;
    }
    mqttJsonObject["mqttServerAddr"] = serverAddrLineEdit->text().trimmed();
    mqttJsonObject["mqttServerPort"] = port;
    mqttJsonObject["mqttUsername"] = usernameLineEdit->text().trimmed();
    mqttJsonObject["mqttPassword"] = passwordLineEdit->text().trimmed();
    writeJsonToFile(mqttConfigFileName, mqttJsonObject);
    this->close();
}

void MqttSettingsWidget::on_quitButton_chicked()
{
    this->close();
}
