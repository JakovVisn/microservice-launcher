#ifndef MICROSERVICE_DATA_H
#define MICROSERVICE_DATA_H

#include "microservice_status.h"

#include <QCheckBox>

class Model;

class MicroserviceData {
public:
    MicroserviceData(const QString name, const QString directory);
    void refreshCheckboxState();
    MicroserviceStatus getStatus() const;
    QString getShortName() const;
    QString getName() const;
    QVector<int> getPorts() const;
    void setCheckBoxChecked(bool checked);
    QCheckBox* getCheckBox();
    QCheckBox* getstatusCheckBox();

private:
    bool isServiceRunning() const;
    bool checkDebug() const;
    int getPid() const;
    QString readApplicationShortNameFromFile(const QString& filePath) const;
    QString getFolderInfo() const;
    QVector<int> readPortsFromFile() const;

    const QString name;
    const QString directory;
    const QString shortName;
    MicroserviceStatus status;
    QCheckBox* checkBox;
    QCheckBox* statusCheckBox;
    const QVector<int> ports;
};

#endif // MICROSERVICE_DATA_H
