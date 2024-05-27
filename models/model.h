#ifndef MODEL_H
#define MODEL_H

#include <QtWidgets/qcheckbox.h>

class MicroserviceData;

class Model
{
public:
    explicit Model();
    QString getConfigFile() const;
    QMap<QString, MicroserviceData*> getMicroservices() const;
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

    QMap<QString, MicroserviceData*> microservices;
};

#endif // MODEL_H
