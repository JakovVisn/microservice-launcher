#include "model.h"
#include "microservice_data.h"

#include <QDir>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QCoreApplication>

Model::Model()
    : defaultConfigFile(createEmptyFile("/config.ini"))
    , defaultSaveFile(createEmptyFile("/save.ini"))
    , directory(findDirectory())
    , defaultButtonStyle(readDefaultButtonStyle())
    , microservices(getFolderNames(), directory)
    , flagNames(loadFlagNames())
{}

QString Model::getConfigFile() const {
    return defaultConfigFile;
}

QString Model::findDirectory() const {
    QString directory = readDirectory();

    QProcess process;
    process.start("sh", QStringList() << "-c" << "echo " + directory);
    process.waitForFinished(-1);
    directory = process.readAllStandardOutput().trimmed();

    QDir initialDir(directory);
    if (directory.isEmpty() || !initialDir.exists()) {
        QMessageBox::critical(nullptr, "Error", "Directory " + directory + " does not exist or is empty.");
        exit(EXIT_FAILURE);
    }

    qDebug() << "initialDir: " << directory;
    return directory;
}

QString Model::readDefaultButtonStyle() const {
    QSettings settings(defaultConfigFile, QSettings::IniFormat);

    settings.beginGroup("Main");

    if (!settings.contains("DefaultButtonStyle")) {
        return "";
    }

    QString buttonStyle = settings.value("DefaultButtonStyle").toString();

    settings.endGroup();

    return buttonStyle;
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

QString Model::getDirectory() const
{
    return directory;
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

MicroserviceDataMap Model::getMicroservices() const {
    return microservices;
}

QString Model::getSaveFile() const {
    return defaultSaveFile;
}

QStringList Model::loadFlagNames() {
    QSettings settings(getSaveFile(), QSettings::IniFormat);
    QStringList names;

    settings.beginGroup("SettingsState");
    if (settings.contains("flags")) {
        names = settings.value("flags").toStringList();
    }

    settings.endGroup();

    return names;
}

QStringList Model::getFlagNames() const {
    return flagNames;
}

QString Model::getDefaultButtonStyle() const {
    return defaultButtonStyle;
}

void Model::addFlagName(const QString& flagName) {
    flagNames.append(flagName);
}
