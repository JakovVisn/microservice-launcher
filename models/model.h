#ifndef MODEL_H
#define MODEL_H

#include <QtWidgets/qcheckbox.h>
#include "microservice_data_map.h"

class MicroserviceData;

class Model
{
public:
    explicit Model();
    QString getConfigFile() const;
    MicroserviceDataMap getMicroservices() const;
    int getPidByName(const QString& processName) const;
    QString getDirectory() const;
    int getProcessID(const QString& processName) const;
    QString getSaveFile() const;

private:
    QString readDirectory() const;
    QStringList readExcludedFoldersFromConfig() const;
    QString createEmptyFile(const QString fileName) const;
    QString findDirectory() const;
    QStringList getFolderNames() const;

    const QString defaultConfigFile;
    const QString defaultSaveFile;
    const QString directory;

    MicroserviceDataMap microservices;
};

#endif // MODEL_H
