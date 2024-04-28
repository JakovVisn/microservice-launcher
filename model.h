#ifndef MODEL_H
#define MODEL_H

#include <QtWidgets/qcheckbox.h>

class Model
{
public:
    explicit Model();
    QString getConfigFile() const;
    QMap<QString, QCheckBox*> getCheckBoxes() const;

private:
    QString readDirectory() const;
    QStringList readExcludedFoldersFromConfig() const;
    QString createEmptyFile(const QString fileName) const;
    QString findDirectory() const;
    QStringList getFolderNames() const;

    const QString defaultConfigFile;
    const QString directory;

    QMap<QString, QCheckBox*> checkBoxes;
};

#endif // MODEL_H
