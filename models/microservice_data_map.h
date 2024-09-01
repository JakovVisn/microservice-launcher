#ifndef MICROSERVICE_DATA_MAP_H
#define MICROSERVICE_DATA_MAP_H

#include "microservice_data.h"

class MicroserviceDataMap {
public:
    MicroserviceDataMap(const QStringList serviceNames, const QString directory);
    MicroserviceData* value(const QString& key) const;
    QVector<MicroserviceData*> getCheckedServices();
    QVector<MicroserviceData*> getServicesByStatus(const MicroserviceStatus& status) const;
    const QMap<QString, MicroserviceData*>& getDataMap() const;
    bool contains(const QString& key) const;

private:
    QMap<QString, MicroserviceData*> dataMap;
};

#endif // MICROSERVICE_DATA_MAP_H
