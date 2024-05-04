#ifndef MODEL_H
#define MODEL_H

#include <QtWidgets/qcheckbox.h>

class Model
{
public:
    explicit Model();
    QString getConfigFile() const;
    QMap<QString, QCheckBox*> getCheckBoxes() const;
    bool checkDebug(const QString& processName) const;
    int getPidByName(const QString& processName) const;
    QString getDirectory() const;
    QString getShortNameByName(const QString fileName) const;
    bool isServiceRunning(const QString& processName) const;
    QMap<QString, QCheckBox*> getCheckBoxStatuses() const;
    int getProcessId(const QString& processName) const;

private:
    QString getFolderInfo(const QString& folderName) const;
    QString readDirectory() const;
    QStringList readExcludedFoldersFromConfig() const;
    QString createEmptyFile(const QString fileName) const;
    QString readApplicationShortNameFromFile(const QString& filePath) const;
    QVector<int> getServicePorts(const QString& folderName) const;
    QString findDirectory() const;
    QStringList getFolderNames() const;

    const QString defaultConfigFile;
    const QString directory;

    QMap<QString, QCheckBox*> checkBoxes;
    QMap<QString, QCheckBox*> checkBoxStatuses;
    QMap<QString, QString> shortNames;
    QMap<QString, QVector<int>> servicePorts;
};

#endif // MODEL_H
