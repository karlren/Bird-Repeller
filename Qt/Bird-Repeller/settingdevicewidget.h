#ifndef SETTINGDEVICEWIDGET_H
#define SETTINGDEVICEWIDGET_H

#include <QWidget>
#include "deviceConfig.h"

class MainWindow;

namespace Ui {
class SettingDeviceWidget;
}

class SettingDeviceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingDeviceWidget(MainWindow *mainWindow, QWidget *parent = nullptr);
    ~SettingDeviceWidget();
    bool deviceDataAnalysis(QJsonObject result);
    bool setDeviceID(int deviceID);

private slots:
    void on_importButton_clicked();

    void on_inquireButton_clicked();

    void on_saveButton_clicked();

    void on_returnButton_clicked();

private:
    Ui::SettingDeviceWidget *ui;
    MainWindow *mainWindow;
    QString wifiSsid[MAX_WIFI_STORAGE], wifiPassword[MAX_WIFI_STORAGE];
    QString mqttUsername, mqttPassword, mqttServerIp, mqttServerPort;

};



#endif // SETTINGDEVICEWIDGET_H
