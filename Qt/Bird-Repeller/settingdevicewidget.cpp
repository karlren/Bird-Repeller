#include "settingdevicewidget.h"
#include "ui_settingdevicewidget.h"
#include "mainwindow.h"

SettingDeviceWidget::SettingDeviceWidget(MainWindow *mainWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingDeviceWidget),
    mainWindow(mainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("配置设备信息");
    /* 设置控件字体和大小 */
    QFont windowFont("得意黑", 14);
    setFont(windowFont);
}

SettingDeviceWidget::~SettingDeviceWidget()
{
    delete ui;
}

bool SettingDeviceWidget::deviceDataAnalysis(QJsonObject result)
{
    QString wifiSsid[MAX_WIFI_STORAGE], wifiPassword[MAX_WIFI_STORAGE];
    QJsonObject content = result["content"].toObject();
    QJsonObject mqttConfigs = content["mqttConfig"].toObject();
    QString mqttUsername = mqttConfigs["mqttUsername"].toString();
    QString mqttPassword = mqttConfigs["mqttPassword"].toString();
    QString mqttHost = mqttConfigs["mqttHost"].toString();
    QString mqttPort = mqttConfigs["mqttPort"].toString();
    QJsonArray wifiConfigsArray = content["wifiConfigs"].toArray();
    for (const QJsonValue &value : wifiConfigsArray) {
        QJsonObject wifiConfig = value.toObject();
        int id = wifiConfig["id"].toInt();  // 获取 id
        if (id > MAX_WIFI_STORAGE) {
            continue;
        }
        QString ssid = wifiConfig["SSID"].toString();  // 获取 SSID
        QString password = wifiConfig["password"].toString();  // 获取密码
        wifiSsid[id] = ssid;
        wifiPassword[id] = password;

        QString ssidLineEditName = QString("wifiSSIDLineEdit_%1").arg(id);
        QString passwordLineEditName = QString("wifiPasswordLineEdit_%1").arg(id);

        QLineEdit *ssidLineEdit = this->findChild<QLineEdit *>(ssidLineEditName);
        QLineEdit *passwordLineEdit = this->findChild<QLineEdit *>(passwordLineEditName);

        // 如果找到了对应的 QLineEdit 控件，设置文本
        if (ssidLineEdit) {
            ssidLineEdit->setText(ssid);
        }
        if (passwordLineEdit) {
            passwordLineEdit->setText(password);
        }

    }


    ui->mqttUsernameLineEdit->setText(mqttUsername);
    ui->mqttPasswordLineEdit->setText(mqttPassword);
    ui->mqttServerIpLineEdit->setText(mqttHost);
    ui->mqttServerPortLineEdit->setText(mqttPort);

    return true;
}

bool SettingDeviceWidget::setDeviceID(int deviceID)
{
    setWindowTitle("配置设备" + QString::number(deviceID) + "信息");
    ui->deviceIDLabel->setText("设备" + QString::number(deviceID));
    return true;
}

void SettingDeviceWidget::on_importButton_clicked()
{
//    QJsonObject
}

void SettingDeviceWidget::on_inquireButton_clicked()
{
    QJsonObject params;
    params["way"] = "read";
    mainWindow->send_mqtt_command("configDeviceInformation", params, DEVICE_CONFIG_QUERY);
}

void SettingDeviceWidget::on_saveButton_clicked()
{
    QString wifiSsid[MAX_WIFI_STORAGE], wifiPassword[MAX_WIFI_STORAGE];
    QJsonObject params;
    params["way"] = "write";
    QJsonObject content;
    QJsonObject mqttConfig;
    QJsonArray wifiConfigs;
    mqttConfig["mqttUsername"] = ui->mqttUsernameLineEdit->text().trimmed();
    mqttConfig["mqttPassword"] = ui->mqttPasswordLineEdit->text().trimmed();
    mqttConfig["mqttHost"] = ui->mqttServerIpLineEdit->text().trimmed();
    mqttConfig["mqttPort"] = ui->mqttServerPortLineEdit->text().trimmed();

    for (int i = 0; i < MAX_WIFI_STORAGE; i++) {
        QJsonObject wifiConfig;
        QString ssidLineEditName = QString("wifiSSIDLineEdit_%1").arg(i);
        QString passwordLineEditName = QString("wifiPasswordLineEdit_%1").arg(i);

        QLineEdit *ssidLineEdit = this->findChild<QLineEdit *>(ssidLineEditName);
        QLineEdit *passwordLineEdit = this->findChild<QLineEdit *>(passwordLineEditName);
        // 找到对应的 QLineEdit 控件
        if (!ssidLineEdit || !passwordLineEdit) {
            continue;
        }
        if (ssidLineEdit->text().trimmed().isEmpty()) {
            continue;
        }
        if (passwordLineEdit->text().trimmed().isEmpty()) {
            continue;
        }
        wifiConfig["SSID"] = ssidLineEdit->text().trimmed();
        wifiConfig["password"] = passwordLineEdit->text().trimmed();
        wifiConfig["id"] = i;
        wifiConfigs.append(wifiConfig);
    }

    content["mqttConfig"] = mqttConfig;
    content["wifiConfigs"] = wifiConfigs;
    params["content"] = content;
    mainWindow->send_mqtt_command("configDeviceInformation", params);
}

void SettingDeviceWidget::on_returnButton_clicked()
{
    this->hide();
}
