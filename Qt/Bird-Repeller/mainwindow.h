#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mqtt/qmqttclient.h>
#include <QMessageBox>          /* 弹窗警告 */
#include <QJsonObject>
#include <QJsonDocument>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QDebug>
#include "mqttsettingswidget.h"
#include <QSpinBox>             /* 数值输入框 */
#include <QDoubleSpinBox>
#include <QComboBox>
#include <cmath>
#include <QGroupBox>
#include <QComboBox>
#include <QPlainTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    MqttSettingsWidget *mqttSettingsWidget;

    QPushButton *loadingButton;             /* 装弹按钮 */
    QPushButton *resettingButton;           /* 复位按钮 */
    QPushButton *connectButton;             /* MQTT连接按钮 */
    QPushButton *inquireButton;             /* 查询角度按钮 */
    QPushButton *deviceQueryButton;         /* 设备查询按钮 */

    QButtonGroup *PTcontrolButtonGruop;     /* 云台控制按键组 */
    QPushButton *upButton;                  /* 云台控制向上按键 */
    QPushButton *downButton;                /* 云台控制向下按键 */
    QPushButton *leftButton;                /* 云台控制向左按键 */
    QPushButton *rightButton;               /* 云台控制向右按键 */
    QPushButton *stopButton;                /* 云台停止按键 */

    QPushButton *PTPresetPointSetButton;    /* 云台预设点设置按键 */
    QPushButton *PTPresetPointCallButton;   /* 云台预设点调用按键 */
    QPushButton *PTPresetPointDeleteButton; /* 云台预设点删除按键 */

    QPushButton *PTSetPanAngleButton;       /* 云台水平角度设置 */
    QPushButton *PTSetTiltAngleButton;      /* 云台俯仰角度设置 */

    QGroupBox *deviceGroupBox;

    QComboBox *deviceComboBox;              /* 设备选择下拉 */

    QLabel *chooseDeviceLabel;              /* 选择设备标签 */
    QLabel *panAngleLabel;                  /* 云台水平轴角度显示标签 */
    QLabel *tiltAngleLabel;                 /* 云台俯仰轴角度显示标签 */
    QLabel *PTcontrolSpeedLabel;            /* 云台速度控制标签 */
    QLabel *PTPresetPointLabel;             /* 云台预设点标签 */
    QLabel *PTSetPanAngleLabel;             /* 云台水平角度设置标签   */
    QLabel *PTSetTiltAngleLabel;            /* 云台俯仰角度设置标签 */
    QLabel *messageReturnLabel;             /* MQTT消息返回标签 */

    QSpinBox *PTcontrolSpeedSpinBox;        /* 云台速度控制数字框 */
    QSpinBox *PTPresetPointSpinBox;         /* 云台预设点数字框 */

    QDoubleSpinBox *PTSetPanAngleDoubleSpinBox; /* 云台水平角度设置浮点数数据框 */
    QDoubleSpinBox *PTSetTiltAngleDoubleSpinBox;/* 云台俯仰角度设置浮点数数据框 */

    QPlainTextEdit *recvMessagesPlainTextEdit;  /* 接收MQTT消息展示框 */
    QPushButton *clearRecvMessageButton;

    bool send_mqtt_command(const QString &method, QJsonObject params = QJsonObject(), int id = 0);
    bool send_mqtt_command(const QString &method, int id);

protected:
    QMqttClient *pMqttClient;
    MqttSettingsWidget *pMqttSettingsWidget;
    /* 声明 keyPressEvent */
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void updateLogStateChange();    /* 客户端状态发生改变 */
    void recv_message(const QByteArray &message, const QMqttTopicName &topic);  /* 客户端接受消息 */
    void openMqttSettingsWidget();
    void connectButton_clicked();
    void loadingButton_clicked();
    void resettingButton_clicked();
    void inquireButton_clicked();
    void deviceButton_clicked(int id);
    void PTcontrolButton_clicked(int id);
    void PTPresetPointSetButton_clicked();
    void PTPresetPointCallButton_clicked();
    void PTPresetPointDeleteButton_clicked();
    void PTSetPanAngleButton_clicked();
    void PTSetTiltAngleButton_clicked();
    void deviceComboBox_indexChanged(int index);
    void deviceQueryButton_chicked();
};

struct DeviceAngle {
    double panAngle;
    double tiltAngle;
};

#endif // MAINWINDOW_H
