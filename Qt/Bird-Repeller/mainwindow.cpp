#include "mainwindow.h"
#include "ui_mainwindow.h"

struct DeviceAngle g_deviceAngles[10];

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    pMqttSettingsWidget = new MqttSettingsWidget();
    pSettingDeviceWidget = new SettingDeviceWidget(this);

    ui->setupUi(this);

    this->resize(1500, 600);        /* 设置窗口大小 */
    this->setMinimumSize(1500, 600);
    this->setMaximumSize(1500, 600);
    setWindowTitle("驱鸟炮塔控制");   /* 设置窗口名称 */
    /* 创建菜单栏 */
    QMenuBar *menuBar = this->menuBar();
    /* 创建菜单项 */
    QAction *mqttSettingsAction = menuBar->addAction("MQTT配置");
    connect(mqttSettingsAction, &QAction::triggered, this, &MainWindow::openMqttSettingsWidget);
    QAction *settingsDeviceAction = menuBar->addAction("查询设备配置信息");
    connect(settingsDeviceAction, &QAction::triggered, this, &MainWindow::openSettingsDeviceWidget);

    /* 设置控件字体和大小 */
    QFont windowFont("得意黑", 16);
    setFont(windowFont);

    /* 创建一个客户端对象 */
    pMqttClient = new QMqttClient(this);

    /* MQTT连接按键 */
    connectButton = new QPushButton("连接断开", this);
    connectButton->setGeometry(1000, 100, 150, 80);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectButton_clicked);

    /* 云台装弹按钮 */
    loadingButton = new QPushButton("装弹", this);
    loadingButton->setGeometry(270, 100, 150, 80);
    connect(loadingButton, &QPushButton::clicked, this, &MainWindow::loadingButton_clicked);

    /* 装弹复位按钮 */
    resettingButton = new QPushButton("装弹复位", this);
    resettingButton->setGeometry(270, 200, 150, 80);
    connect(resettingButton, &QPushButton::clicked, this, &MainWindow::resettingButton_clicked);

    /* 点火按钮 */
    ignitionButton = new QPushButton("点火", this);
    ignitionButton->setGeometry(270, 300, 150, 80);
    connect(ignitionButton, &QPushButton::clicked, this, &MainWindow::ignitionButton_clicked);

    /* 清理炮管按钮 */
    cleanBarrelButton = new QPushButton("清理炮管", this);
    cleanBarrelButton->setGeometry(270, 400, 150, 80);
    connect(cleanBarrelButton, &QPushButton::clicked, this, &MainWindow::cleanBarrelButton_clicked);

    /* 云台自检按钮 */
    SelfTestButton = new QPushButton("云台自检", this);
    SelfTestButton->setGeometry(430, 100, 150, 80);
    connect(SelfTestButton, &QPushButton::clicked, this, &MainWindow::SelfTestButton_clicked);

    /* 设备复位按钮 */
    deviceResetButton = new QPushButton("esp32复位", this);
    deviceResetButton->setGeometry(270, 500, 150, 80);
    connect(deviceResetButton, &QPushButton::clicked, this, &MainWindow::deviceResetButton_clicked);

    /* 角度查询按钮 */
    inquireButton = new QPushButton("角度查询", this);
    inquireButton->setGeometry(600, 100, 150, 80);
    connect(inquireButton, &QPushButton::clicked, this, &MainWindow::inquireButton_clicked);
    panAngleLabel = new QLabel("水平角度：0", this);
    panAngleLabel->setGeometry(800, 90, 300, 50);
    tiltAngleLabel = new QLabel("俯仰角度：0", this);
    tiltAngleLabel->setGeometry(800, 150, 300, 50);

    /* 设备选择下拉框 */
    deviceComboBox = new QComboBox(this);
    deviceComboBox->addItem("所有设备");
    for (int i = 1; i <= DEVICE_NUMBER; ++i) {
        deviceComboBox->addItem(QString("设备%1").arg(i));
    }
    deviceComboBox->setGeometry(50, 130, 180, 50);
    deviceComboBox->setCurrentIndex(2);
//    connect(deviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::deviceComboBox_indexChanged);

    /* 设备查询按键 */
    deviceQueryButton = new QPushButton("查询在线设备", this);
    deviceQueryButton->setGeometry(50, 225, 180, 80);
//    deviceQueryButton->hide();
    connect(deviceQueryButton, &QPushButton::clicked, this, &MainWindow::deviceQueryButton_chicked);

    /* 云台控制按键 */
    upButton = new QPushButton("↑", this);
    downButton = new QPushButton("↓", this);
    leftButton = new QPushButton("←", this);
    rightButton = new QPushButton("→", this);
    stopButton = new QPushButton("停止", this);

    upButton->setGeometry(520, 250, 80, 80);
    downButton->setGeometry(520, 410, 80, 80);
    leftButton->setGeometry(440, 330, 80, 80);
    rightButton->setGeometry(600, 330, 80, 80);
    stopButton->setGeometry(520, 330, 80, 80);

    PTcontrolButtonGruop = new QButtonGroup();
    PTcontrolButtonGruop->addButton(stopButton, 0);
    PTcontrolButtonGruop->addButton(upButton, 1);
    PTcontrolButtonGruop->addButton(downButton, 2);
    PTcontrolButtonGruop->addButton(leftButton, 3);
    PTcontrolButtonGruop->addButton(rightButton, 4);

    /* 云台预设点按键 */
    PTPresetPointSetButton = new QPushButton("云台预设点设置", this);
    PTPresetPointCallButton = new QPushButton("云台预设点调用", this);
    PTPresetPointDeleteButton = new QPushButton("云台预设点删除", this);

    PTPresetPointSetButton->setGeometry(50, 400, 200, 60);
    PTPresetPointCallButton->setGeometry(50, 460, 200, 60);
    PTPresetPointDeleteButton->setGeometry(50, 520, 200, 60);

    connect(PTPresetPointSetButton, &QPushButton::clicked, this, &MainWindow::PTPresetPointSetButton_clicked);
    connect(PTPresetPointCallButton, &QPushButton::clicked, this, &MainWindow::PTPresetPointCallButton_clicked);
    connect(PTPresetPointDeleteButton, &QPushButton::clicked, this, &MainWindow::PTPresetPointDeleteButton_clicked);

    /* 标签 */
    chooseDeviceLabel = new QLabel("选择设备", this);
    chooseDeviceLabel->setGeometry(50, 80, 180, 50);
    PTcontrolSpeedLabel = new QLabel("速度控制：", this);
    PTcontrolSpeedLabel->setGeometry(440, 500, 150, 50);
    PTPresetPointLabel = new QLabel("预设点：", this);
    PTPresetPointLabel->setGeometry(50, 350, 100, 50);

    /* 云台预设点设置数字输入框 */
    PTPresetPointSpinBox = new QSpinBox(this);
    PTPresetPointSpinBox->setGeometry(150, 350, 100, 50);
    PTPresetPointSpinBox->setValue(2);
    PTPresetPointSpinBox->setMinimum(0);
    PTPresetPointSpinBox->setMaximum(0xFF);
    PTPresetPointSpinBox->setAccelerated(true);

    /* 云台速度控制滑动框 */
    PTcontrolSpeedSpinBox = new QSpinBox(this);
    PTcontrolSpeedSpinBox->setGeometry(560, 500, 100, 50);
    PTcontrolSpeedSpinBox->setValue(0x20);
    PTcontrolSpeedSpinBox->setMinimum(0);
    PTcontrolSpeedSpinBox->setMaximum(0x40);
    PTcontrolSpeedSpinBox->setAccelerated(true);

    /* 云台水平、俯仰角度控制 */
    PTSetPanAngleLabel = new QLabel("云台水平角度：", this);
    PTSetPanAngleLabel->setGeometry(720, 220, 200, 60);
    PTSetPanAngleDoubleSpinBox = new QDoubleSpinBox(this);
    PTSetPanAngleDoubleSpinBox->setGeometry(880, 220, 150, 60);
    PTSetPanAngleDoubleSpinBox->setRange(0.00, 120.00);
    PTSetPanAngleDoubleSpinBox->setSingleStep(0.5);
    PTSetPanAngleDoubleSpinBox->setValue(60.0);
    PTSetPanAngleButton = new QPushButton("设置", this);
    PTSetPanAngleButton->setGeometry(1050, 220, 120, 60);

    PTSetTiltAngleLabel = new QLabel("云台俯仰角度：", this);
    PTSetTiltAngleLabel->setGeometry(720, 280, 200, 60);
    PTSetTiltAngleDoubleSpinBox = new QDoubleSpinBox(this);
    PTSetTiltAngleDoubleSpinBox->setGeometry(880, 280, 150, 60);
    PTSetTiltAngleDoubleSpinBox->setRange(0.00, 90.00);
    PTSetTiltAngleDoubleSpinBox->setSingleStep(0.5);
    PTSetTiltAngleDoubleSpinBox->setValue(0.0);
    PTSetTiltAngleButton = new QPushButton("设置", this);
    PTSetTiltAngleButton->setGeometry(1050, 280, 120, 60);

    /* 云台水平角度控制滑动条 */
    PTSetPanAngleSlider = new QSlider(Qt::Horizontal, this);
    PTSetPanAngleSlider->setGeometry(1210, 100, 200, 20);
    PTSetPanAngleSlider->setRange(0.0, 120.0);
    PTSetPanAngleSlider->setSingleStep(1);
    PTSetPanAngleSlider->setTracking(false);
    PTSetPanAngleSlider->setValue(60.0);
    connect(PTSetPanAngleSlider, &QSlider::sliderMoved, this, [this](int value){
        PTSetPanAngleDoubleSpinBox->setValue(value);
    });
    connect(PTSetPanAngleSlider, &QSlider::valueChanged, this, [this](int value){
        PTSetPanAngleDoubleSpinBox->setValue(value);
    });
    connect(PTSetPanAngleDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) {
        PTSetPanAngleSlider->setValue(static_cast<int>(value));
    });
    connect(PTSetPanAngleSlider, &QSlider::sliderReleased, this, &MainWindow::PTSetPanAngleButton_clicked);

    /* 云台俯仰角度控制滑动条 */
    PTSetTiltAngleSlider = new QSlider(Qt::Vertical, this);
    PTSetTiltAngleSlider->setGeometry(1300, 140, 20, 200);
    PTSetTiltAngleSlider->setRange(0.0, 90.0);
    PTSetTiltAngleSlider->setTracking(false);
    PTSetTiltAngleSlider->setInvertedAppearance(true);
    PTSetTiltAngleSlider->setValue(0.0);
    connect(PTSetTiltAngleSlider, &QSlider::sliderMoved, this, [this](int value){
        PTSetTiltAngleDoubleSpinBox->setValue(value);
    });
    connect(PTSetTiltAngleSlider, &QSlider::valueChanged, this, [this](int value){
        PTSetTiltAngleDoubleSpinBox->setValue(value);
    });
    connect(PTSetTiltAngleDoubleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double value) {
        PTSetTiltAngleSlider->setValue(static_cast<int>(value));
    });
    connect(PTSetTiltAngleSlider, &QSlider::sliderReleased, this, &MainWindow::PTSetTiltAngleButton_clicked);

    /* 消息返回 */
    messageReturnLabel = new QLabel("消息返回", this);
    messageReturnLabel->setGeometry(750, 380, 150, 50);
    recvMessagesPlainTextEdit = new QPlainTextEdit(this);
    recvMessagesPlainTextEdit->setGeometry(750, 430, 400, 150);
    QFont recvMessagesFont;
    recvMessagesFont.setPointSize(10);    // 设置字体大小为 10pt
    recvMessagesPlainTextEdit->setFont(recvMessagesFont);
    clearRecvMessageButton = new QPushButton("清除", this);
    clearRecvMessageButton->setGeometry(900, 380, 100, 50);
    connect(clearRecvMessageButton, &QPushButton::clicked,
            recvMessagesPlainTextEdit, &QPlainTextEdit::clear);


    connect(PTSetPanAngleButton, &QPushButton::clicked, this, &MainWindow::PTSetPanAngleButton_clicked);
    connect(PTSetTiltAngleButton, &QPushButton::clicked, this, &MainWindow::PTSetTiltAngleButton_clicked);


    shootButton = new QPushButton("射击", this);
    shootButton->setGeometry(1250, 500, 150, 80);
    connect(shootButton, &QPushButton::clicked, this, &MainWindow::shootButton_clicked);

    ignitionTimeCheckBox = new QCheckBox("点火开关使能", this);
    ignitionTimeCheckBox->setGeometry(1300, 390, 200, 50);
    ignitionTimeCheckBox->setChecked(true);

    ignitionTimeLineEdit = new QLineEdit("50", this);
    ignitionTimeLineEdit->setGeometry(1180, 390, 100, 50);

    cleanTimeCheckBox = new QCheckBox("气泵使能", this);
    cleanTimeCheckBox->setGeometry(1300, 440, 200, 50);
    cleanTimeCheckBox->setChecked(true);

    cleanTimeLineEdit = new QLineEdit("1000", this);
    cleanTimeLineEdit->setGeometry(1180, 440, 100, 50);

    waitTimeCheckBox = new QCheckBox("击发等待", this);
    waitTimeCheckBox->setGeometry(1300, 340, 200, 50);
    waitTimeCheckBox->setChecked(false);

    waitTimeLineEdit = new QLineEdit("1000", this);
    waitTimeLineEdit->setGeometry(1180, 340, 100, 50);


    /* 发送命令 */
//    sendCommandMethodLineEdit = new QLineEdit(this);
//    sendCommandMethodLineEdit->setGeometry(1200, 360, 200, 50);
//    sendCommandparamsTextEdit = new QTextEdit(this);
//    sendCommandparamsTextEdit->setGeometry(1200, 430, 200, 150);

    /* 连接客户端状态变化信号 */
    connect(pMqttClient, &QMqttClient::stateChanged, this, &MainWindow::updateLogStateChange);
    /* 连接客户端订阅消息信号 */
    connect(pMqttClient, &QMqttClient::messageReceived, this, &MainWindow::recv_message);
    /* 连接云台控制按键组信号 */
    connect(PTcontrolButtonGruop, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::PTcontrolButton_clicked);

    connectButton_clicked();
}

MainWindow::~MainWindow()
{
    pMqttClient->disconnectFromHost();
    pMqttSettingsWidget->close();
    delete pMqttSettingsWidget;
    pSettingDeviceWidget->close();
    delete pSettingDeviceWidget;
    delete ui;
}

bool MainWindow::send_mqtt_command(const QString &method, QJsonObject params, int id)
{
    QString topic = "esp32/control";
    int deviceID = deviceComboBox->currentIndex();
    if (deviceID == -1) {
        QMessageBox::warning(NULL, "警告", "未选择设备");
        return false;
    }

    if (pMqttClient->state() == QMqttClient::Connected) {
        QJsonObject command;
        command["deviceID"] = deviceID;
        command["method"] = method;
        command["params"] = params;
        command["id"] = id;
        QJsonDocument doc(command);
        pMqttClient->publish(QMqttTopicName(topic), doc.toJson());
    } else {
        QMessageBox::warning(NULL, "警告", "请先连接服务器");
        return false;
    }
    return true;

}

bool MainWindow::send_mqtt_command(const QString &method, int id)
{
    return send_mqtt_command(method, QJsonObject(), id);
}

void MainWindow::updateLogStateChange()
{
    if (pMqttClient->state() == QMqttClient::Disconnected) {
        connectButton->setText("连接断开");
    } else if (pMqttClient->state() == QMqttClient::Connecting) {
        connectButton->setText("正在连接中");
    } else if (pMqttClient->state() == QMqttClient::Connected) {
        connectButton->setText("已连接");
        pMqttClient->subscribe(QMqttTopicFilter("esp32/result"));   /* 订阅主题 */
        pMqttClient->subscribe(QMqttTopicFilter("esp32/error"));   /* 订阅主题 */
        pMqttClient->subscribe(QMqttTopicFilter("esp32/deviceList"));   /* 订阅主题 */
    }
}

void MainWindow::recv_message(const QByteArray &message, const QMqttTopicName &topic)
{
//    qDebug()<<message<<"\n"<<topic.name();
    int deviceID = deviceComboBox->currentIndex();      /* 获取当前选择的设备 */
    if (topic.name() == "esp32/result") {
        QJsonDocument recvMessageDoc = QJsonDocument::fromJson(message);
        if (!recvMessageDoc.isNull() && recvMessageDoc.isObject()) {
            QJsonObject result = recvMessageDoc.object();
            if (result.contains("message")) {
                QString resultMessage = "---> " + result["message"].toString();
                recvMessagesPlainTextEdit->appendPlainText(resultMessage);
            }
            if (result.contains("id")) {
                int id = result["id"].toInt();
                switch (id) {   /* 根据JSON中的id键值判断是哪个任务的返回 */
                case DEVICE_CONFIG_QUERY:
                    if (pSettingDeviceWidget->deviceDataAnalysis(result)) {
                        recvMessagesPlainTextEdit->appendPlainText("解析设备配置信息成功");
                        pSettingDeviceWidget->setDeviceID(deviceComboBox->currentIndex());
                        pSettingDeviceWidget->show();
                    } else {
                        recvMessagesPlainTextEdit->appendPlainText("解析设备配置信息失败");
                    }
                    break;
                case ANGLE_QUERY_ID:    /* 如果是查询角度命令 */
                    g_deviceAngles[result["deviceID"].toInt()].panAngle = result["panAngle"].toDouble();
                    g_deviceAngles[result["deviceID"].toInt()].tiltAngle = result["tiltAngle"].toDouble();
                    if (deviceID == result["deviceID"].toInt()) {
                        panAngleLabel->setText("水平角度：" + QString::number(g_deviceAngles[deviceID].panAngle, 'f', 2));
                        tiltAngleLabel->setText("俯仰角度：" + QString::number(g_deviceAngles[deviceID].tiltAngle, 'f', 2));
                    }
                    break;
                default:
                    break;
                }
            }
            if (result.contains("IP")) {
                QString message = "设备" + QString::number(result["deviceID"].toInt()) + "在线。";
                message += "版本为" + result["edition"].toString();
                message += ",IP地址：" + result["IP"].toString();
                recvMessagesPlainTextEdit->appendPlainText(message);
            }
        }
    } else if (topic.name() == "esp32/error") {
        QJsonDocument recvMessageDoc = QJsonDocument::fromJson(message);
        if (recvMessageDoc.isNull() || !recvMessageDoc.isObject()) {
            return;
        }
        QJsonObject error = recvMessageDoc.object();
        QJsonObject errorObj = error["error"].toObject();
        QString errorMessage = "设备" + QString::number(error["deviceID"].toInt()) + "错误：" + errorObj["message"].toString();
        recvMessagesPlainTextEdit->appendPlainText(errorMessage);
    } else if (topic.name() == "esp32/deviceList") {
        QJsonDocument recvMessageDoc = QJsonDocument::fromJson(message);
        if (recvMessageDoc.isNull() || !recvMessageDoc.isObject()) {
            return;
        }
        QJsonObject result = recvMessageDoc.object();
        QString message = "设备" + QString::number(result["deviceID"].toInt()) + "已上线。";
        message += "版本为" + result["edition"].toString();
        message += ",IP地址：" + result["IP"].toString();
        recvMessagesPlainTextEdit->appendPlainText(message);
    }
}

void MainWindow::openMqttSettingsWidget()
{
    pMqttSettingsWidget->show();
}

void MainWindow::openSettingsDeviceWidget()
{
    QJsonObject params;
    params["way"] = "read";
    send_mqtt_command("configDeviceInformation", params, DEVICE_CONFIG_QUERY);
//    pSettingDeviceWidget->show();
}

void MainWindow::connectButton_clicked()
{
    pMqttClient->setHostname(pMqttSettingsWidget->getServerAddr());     /* 设置主机IP */
    pMqttClient->setPort(pMqttSettingsWidget->getServerPort()); /* 设置端口号 */
    if (pMqttClient->state() == QMqttClient::Disconnected) {
        pMqttClient->setUsername(pMqttSettingsWidget->getUsername());
        pMqttClient->setPassword(pMqttSettingsWidget->getPassword());
        /* 设置遗嘱 */
        pMqttClient->setWillTopic("QT/will");
        pMqttClient->setWillQoS(0);
        pMqttClient->setWillMessage("{\"message\":\"Windows QT Unexpected disconnection\"}");
        pMqttClient->setWillRetain(0);

        // 连接到 MQTT 主机
        pMqttClient->connectToHost();
    } else {
        pMqttClient->disconnectFromHost();
    }
}

void MainWindow::loadingButton_clicked()
{
    send_mqtt_command("loading");
}

void MainWindow::resettingButton_clicked()
{
    send_mqtt_command("StepperHoming");
}

void MainWindow::inquireButton_clicked()
{
    send_mqtt_command("getPTAngle", ANGLE_QUERY_ID);
}

void MainWindow::ignitionButton_clicked()
{
    send_mqtt_command("ignition");
}

void MainWindow::cleanBarrelButton_clicked()
{
    send_mqtt_command("cleanBarrel");
}

void MainWindow::SelfTestButton_clicked()
{
    QJsonObject params;
    params["command"] = 1;
    send_mqtt_command("PanTiltControl", params);
}

void MainWindow::deviceResetButton_clicked()
{
    send_mqtt_command("reset");
}

void MainWindow::deviceButton_clicked(int id)
{
    if (id == 0) {
        inquireButton->setVisible(false);
        panAngleLabel->setVisible(false);
        tiltAngleLabel->setVisible(false);

    } else {
        inquireButton->setVisible(true);
        panAngleLabel->setVisible(true);
        tiltAngleLabel->setVisible(true);
        panAngleLabel->setText("水平角度：" + QString::number(g_deviceAngles[id].panAngle));
        tiltAngleLabel->setText("俯仰角度：" + QString::number(g_deviceAngles[id].tiltAngle));
    }
}

void MainWindow::PTcontrolButton_clicked(int id)
{
    QJsonObject params;
    switch (id) {
    case 0:
        params["command"] = 8;
        break;
    case 1:
        params["command"] = 6;
        params["param1"] = PTcontrolSpeedSpinBox->value();
        params["param2"] = PTcontrolSpeedSpinBox->value();
        break;
    case 2:
        params["command"] = 7;
        params["param1"] = PTcontrolSpeedSpinBox->value();
        params["param2"] = PTcontrolSpeedSpinBox->value();
        break;
    case 3:
        params["command"] = 5;
        params["param1"] = PTcontrolSpeedSpinBox->value();
        params["param2"] = PTcontrolSpeedSpinBox->value();
        break;
    case 4:
        params["command"] = 4;
        params["param1"] = PTcontrolSpeedSpinBox->value();
        params["param2"] = PTcontrolSpeedSpinBox->value();
        break;
    }
    send_mqtt_command("PanTiltControl", params);
}

void MainWindow::PTPresetPointSetButton_clicked()
{
    QString confirmationMessage = "你确认要调用预设点" + QString::number(PTPresetPointSpinBox->value()) + "吗？";
    QMessageBox::StandardButton response = QMessageBox::question(nullptr, "请确认", confirmationMessage,
                                                                QMessageBox::Yes | QMessageBox::No);
    if (response == QMessageBox::Yes) {
        QJsonObject params;
        params["command"] = 11;
        params["param2"] = PTPresetPointSpinBox->value();
        send_mqtt_command("PanTiltControl", params);
    }
}

void MainWindow::PTPresetPointCallButton_clicked()
{
    QJsonObject params;
    params["command"] = 12;
    params["param2"] = PTPresetPointSpinBox->value();
    send_mqtt_command("PanTiltControl", params);
}

void MainWindow::PTPresetPointDeleteButton_clicked()
{
    QString confirmationMessage = "你确认要删除预设点" + QString::number(PTPresetPointSpinBox->value()) + "吗？";
    QMessageBox::StandardButton response = QMessageBox::question(nullptr, "请确认", confirmationMessage,
                                                                QMessageBox::Yes | QMessageBox::No);
    if (response == QMessageBox::Yes) {
        QJsonObject params;
        params["command"] = 13;
        params["param2"] = PTPresetPointSpinBox->value();
        send_mqtt_command("PanTiltControl", params);
    }
}

void MainWindow::PTSetPanAngleButton_clicked()
{
    double value = PTSetPanAngleDoubleSpinBox->value();
    if (value >= 0.0f) {
        value /= 2.56f;
    } else {
        value += 360.0f;
        value /= 2.56f;
    }


    int integerPart = static_cast<int>(std::floor(value));
    int decimalPart = static_cast<int>((value - integerPart) * 100);
    QJsonObject params;
    params["command"] = 20;
    params["param1"] = integerPart;
    params["param2"] = decimalPart;

    send_mqtt_command("PanTiltControl", params);
}

void MainWindow::PTSetTiltAngleButton_clicked()
{
    double value = PTSetTiltAngleDoubleSpinBox->value() / 2.56f;
    int integerPart = static_cast<int>(std::floor(value));
    int decimalPart = static_cast<int>((value - integerPart) * 100);
    QJsonObject params;
    params["command"] = 21;
    params["param1"] = integerPart;
    params["param2"] = decimalPart;

    send_mqtt_command("PanTiltControl", params);
}

void MainWindow::deviceComboBox_indexChanged(int index)
{
    if (index == 0) {
        inquireButton->setVisible(false);
        panAngleLabel->setVisible(false);
        tiltAngleLabel->setVisible(false);
        deviceQueryButton->setVisible(true);
    } else {
        inquireButton->setVisible(true);
        panAngleLabel->setVisible(true);
        tiltAngleLabel->setVisible(true);
        deviceQueryButton->setVisible(false);
    }
}

void MainWindow::deviceQueryButton_chicked()
{
    send_mqtt_command("getDeviceData");
}

void MainWindow::shootButton_clicked()
{
    QJsonObject params;
    params["command"] = 20;
    params["panAngle"] = PTSetPanAngleDoubleSpinBox->value();
    params["tiltAngle"] = PTSetTiltAngleDoubleSpinBox->value();
    if (waitTimeCheckBox->checkState()) {
        params["waitTime"] = waitTimeLineEdit->text().trimmed().toInt();
    }
    if (ignitionTimeCheckBox->checkState()) {
        params["ignitionTime"] = ignitionTimeLineEdit->text().trimmed().toInt();
    }
    if (cleanTimeCheckBox->checkState()) {
        params["cleanTime"] = cleanTimeLineEdit->text().trimmed().toInt();
    }
    send_mqtt_command("shoot", params);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    /* 按键事件：捕捉上下左右箭头和空格键 */
    int buttonId = -1;  // 记录哪个按钮需要被触发

    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_W) {
        buttonId = 1;
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_S) {
        buttonId = 2;
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_A) {
        buttonId = 3;
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_D) {
        buttonId = 4;
    } else if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_Space) {
        buttonId = 0;
    }

    if (buttonId != -1) {
       /* 模拟按钮点击事件 */
       emit PTcontrolButtonGruop->buttonClicked(buttonId);
    }
    /* 如果有默认的处理操作，可以调用基类的方法 */
       QWidget::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (pSettingDeviceWidget) {
        pSettingDeviceWidget->close();
    }
    if (pMqttSettingsWidget) {
        pMqttSettingsWidget->close();
    }
    event->accept();
}
