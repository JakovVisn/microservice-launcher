#ifndef MICROSERVICE_DATA_H
#define MICROSERVICE_DATA_H

#include "microservice_status.h"

#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>

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
    QCheckBox* getStatusCheckBox();
    QHBoxLayout* getFlagsLayout() const;
    void addFlag(const QString flag, bool visible, bool check = false);
    void setFlagsVisible(bool visible);
    QStringList getEnabledFlags() const;
    QVector<QCheckBox*> getFlagCheckBoxes() const;
    QLabel* getEnabledFlagsLabel() const;
    void updateFlagState(const QString flag, const Qt::CheckState state) const;
    int getPid() const;
    QString getPIDByPorts() const;

public slots:
    void updateEnabledFlagsLabel();

private:
    bool isServiceRunning() const;
    bool checkDebug() const;
    QString readApplicationShortNameFromFile(const QString& filePath) const;
    QString getFolderInfo() const;
    QVector<int> readPortsFromFile(const QString directory) const;

    const QString name;
    const QString shortName;
    MicroserviceStatus status;
    QCheckBox* checkBox;
    QCheckBox* statusCheckBox;
    const QVector<int> ports;
    QVector<QCheckBox*> flagCheckBoxes;
    QHBoxLayout *flagsLayout;
    QLabel *enabledFlagsLabel;
};

#endif // MICROSERVICE_DATA_H
