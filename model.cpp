#include "model.h"

#include <QDir>
#include <QSettings>
#include <QCoreApplication>

Model::Model(): defaultConfigFile(createEmptyFile("/config.ini")), directory(findDirectory()) {
    const QStringList folderNames = getFolderNames();
    for (const QString &folderName : folderNames) {
        QCheckBox *checkBox = new QCheckBox(folderName);
        checkBoxes.insert(folderName, checkBox);
    }
}

QString Model::getConfigFile() const {
    return defaultConfigFile;
}

QString Model::findDirectory() const {
    QString directory = readDirectory();
    QDir initialDir(directory);
    if (directory.isEmpty() || !initialDir.exists()) {
        qDebug() << "Directory does not exist or is empty.";
        throw std::runtime_error("Directory does not exist or is empty.");
    }

    qDebug() << "initialDir: " << directory;
    return directory;
}

QStringList Model::getFolderNames() const {
    QStringList folderNames;
    QDir dir(directory);

    QStringList folderList = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList excludedFolders = readExcludedFoldersFromConfig();

    for (const QString& folderName : folderList) {
        if (!excludedFolders.contains(folderName)) {
            folderNames.append(folderName);
        }
    }

    folderNames.sort();
    return folderNames;
}

QMap<QString, QCheckBox*> Model::getCheckBoxes() const {
    return checkBoxes;
}

QString Model::createEmptyFile(const QString fileName) const {
    const QString& appPath = QCoreApplication::applicationDirPath() + fileName;
    QFile file(appPath);

    if (file.exists()) {
        qDebug() << "File already exists:" << appPath;
    } else {
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            qDebug() << "Empty file created:" << appPath;
        } else {
            qDebug() << "Failed to create the file:" << appPath;
        }
    }

    return appPath;
}

QString Model::readDirectory() const {
    QSettings settings(defaultConfigFile, QSettings::IniFormat);

    settings.beginGroup("Main");

    if (!settings.contains("Directory")) {
        return "";
    }

    QString directory = settings.value("Directory").toString();

    settings.endGroup();

    return directory;
}

QStringList Model::readExcludedFoldersFromConfig() const {
    QSettings settings(defaultConfigFile, QSettings::IniFormat);

    settings.beginGroup("Main");

    QStringList excludedFolders;
    if (settings.contains("ExcludedFolders")) {
        excludedFolders = settings.value("ExcludedFolders").toStringList();
    }

    settings.endGroup();

    return excludedFolders;
}
