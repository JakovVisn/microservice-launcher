#ifndef MODEL_H
#define MODEL_H

#include <QtWidgets/qcheckbox.h>
#include "microservice_data_map.h"

class Model
{
public:
    explicit Model();
    QString getConfigFile() const;
    MicroserviceDataMap getMicroservices() const;
    QString getDirectory() const;
    QString getSaveFile() const;
    QStringList getFlagNames() const;
    QString getDefaultButtonStyle() const;
    void addFlagName(const QString& flagName);

private:
    QString readDirectory() const;
    QString readDefaultButtonStyle() const;
    QStringList readExcludedFoldersFromConfig() const;
    QString createEmptyFile(const QString fileName) const;
    QString findDirectory() const;
    QStringList getFolderNames() const;
    QStringList loadFlagNames();

    const QString defaultConfigFile;
    const QString defaultSaveFile;
    const QString directory;
    const QString defaultButtonStyle;

    MicroserviceDataMap microservices;

    QStringList flagNames;
};

#endif // MODEL_H
