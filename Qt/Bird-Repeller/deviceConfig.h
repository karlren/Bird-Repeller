#ifndef DEVICECONFIG_H
#define DEVICECONFIG_H

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

#define DEVICE_NUMBER 5
#define MAX_WIFI_STORAGE 3

typedef enum ID{
    DEVICE_CONFIG_QUERY = 1,
    ANGLE_QUERY_ID,
}ID;

#endif // DEVICECONFIG_H
