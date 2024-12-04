#ifndef MqttSettingsWidget_H
#define MqttSettingsWidget_H

#include <QWidget>
#include <QMessageBox>  /* 弹窗 */
#include <QFile>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QDir>

class MqttSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    MqttSettingsWidget(QWidget *parent = nullptr);
    ~MqttSettingsWidget();
    QString getServerAddr();
    int getServerPort();
    QString getUsername();
    QString getPassword();

private:
    bool readJsonFromFile(const QString &fileName, QJsonObject &jsonObject);
    void writeJsonToFile(const QString &fileName, QJsonObject &jsonObject);

    QJsonObject mqttJsonObject;
    QString mqttConfigFileName;

    QLabel *serverAddrLabel;
    QLabel *serverPortLabel;
    QLabel *usernameLabel;
    QLabel *passwordLabel;

    QLineEdit *serverAddrLineEdit;
    QLineEdit *serverPortLineEdit;
    QLineEdit *usernameLineEdit;
    QLineEdit *passwordLineEdit;

    QPushButton *saveButton;
    QPushButton *quitButton;
private slots:
    void on_saveButton_chicked();
    void on_quitButton_chicked();
};

#endif // MqttSettingsWidget_H
