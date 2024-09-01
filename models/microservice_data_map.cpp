#include "microservice_data_map.h"

MicroserviceDataMap::MicroserviceDataMap(const QStringList serviceNames, const QString directory) {
    for (const QString &name : serviceNames) {
        MicroserviceData *microservice = new MicroserviceData(name, directory);
        dataMap.insert(name, microservice);
    }
}

MicroserviceData* MicroserviceDataMap::value(const QString& key) const {
    return dataMap.value(key);
}

QVector<MicroserviceData*> MicroserviceDataMap::getCheckedServices() {
    QVector<MicroserviceData*> checkedCheckBoxes;

    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = dataMap.constBegin(); iter != dataMap.constEnd(); ++iter) {
        if (iter.value()->getCheckBox()->isChecked()) {
            checkedCheckBoxes.append(iter.value());
        }
    }

    return checkedCheckBoxes;
}

QVector<MicroserviceData*> MicroserviceDataMap::getServicesByStatus(const MicroserviceStatus& status) const {
    QVector<MicroserviceData*> servicesWithStatus;

    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = dataMap.constBegin(); iter != dataMap.constEnd(); ++iter) {
        if (iter.value()->getStatus() == status) {
            servicesWithStatus.append(iter.value());
        }
    }

    return servicesWithStatus;
}

const QMap<QString, MicroserviceData*>& MicroserviceDataMap::getDataMap() const {
    return dataMap;
}

bool MicroserviceDataMap::contains(const QString& key) const {
    return dataMap.contains(key);
}
