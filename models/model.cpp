#include "model.h"
#include "microservice_data.h"

#include <QDir>
#include <QProcessEnvironment>
#include <QSettings>
#include <QCoreApplication>

Model::Model()
    : defaultConfigFile(createEmptyFile("/config.ini"))
    , defaultSaveFile(createEmptyFile("/save.ini"))
    , directory(findDirectory())
{
    const QStringList folderNames = getFolderNames();
    for (const QString &folderName : folderNames) {
        MicroserviceData *microservice = new MicroserviceData(folderName, getDirectory());
        microservices.insert(folderName, microservice);
    }
}

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
        qDebug() << "Directory "<< directory << " does not exist or is empty.";
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

QString Model::getDirectory() const
{
    return directory;
}

int Model::getPidByName(const QString& processName) const {
    QString cmd = "pgrep -x " + processName;
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.toStdString().c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    if (!result.empty()) {
        return std::stoi(result);
    } else {
        return -1; // Return -1 if process is not found
    }
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

QMap<QString, MicroserviceData*> Model::getMicroservices() const {
    return microservices;
}

int Model::getProcessID(const QString& processName) const {
    int pid = getPidByName(processName);
    if (pid != -1) {
        return pid;
    }

    QVector<int> ports = microservices.value(processName)->getPorts();

    if (ports.isEmpty()) {
        qDebug() << "No ports found for folder" << processName;
        return -1;
    }

    for (int port : ports) {
        QString cmd = "lsof -i :" + QString::number(port) + " -t";
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.toStdString().c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        if (!result.empty()) {
            return std::stoi(result);
        }
    }

    return -1; // Return -1 if process is not found on any port
}

QString Model::getSaveFile() const {
    return defaultSaveFile;
}
