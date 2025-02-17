#include "model.h"
#include "microservice_data.h"

#include <QDir>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>

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

    for (auto iter = folderList.constBegin(); iter != folderList.constEnd(); ++iter) {
        if (!excludedFolders.contains(*iter)) {
            folderNames.append(*iter);
        }
    }

    folderNames.sort();
    return folderNames;
}

QString Model::getDirectory() const {
    return directory;
}

QString Model::createEmptyFile(const QString fileName) const {
    QString appPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appPath);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "Failed to create directory:" << appPath;
            return QString();
        }
    }

    QString filePath = appPath + fileName;
    QFile file(filePath);

    if (file.exists()) {
        qDebug() << "File already exists:" << filePath;
    } else {
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            qDebug() << "Empty file created:" << filePath;
        } else {
            qDebug() << "Failed to create the file:" << filePath;
        }
    }

    return filePath;
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
