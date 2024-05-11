#include "model.h"

#include <QDir>
#include <QProcessEnvironment>
#include <QSettings>
#include <QCoreApplication>
#include <QTcpSocket>

Model::Model()
    : defaultConfigFile(createEmptyFile("/config.ini"))
    , defaultSaveFile(createEmptyFile("/save.ini"))
    , directory(findDirectory())
{
    const QStringList folderNames = getFolderNames();
    for (const QString &folderName : folderNames) {
        QString shortName = readApplicationShortNameFromFile(getDirectory() + "/" + folderName);
        shortNames.insert(folderName, shortName);

        QVector<int> ports = getServicePorts(folderName);
        servicePorts.insert(folderName, ports);

        QString folderInfo = getFolderInfo(folderName);
        QCheckBox *checkBox = new QCheckBox(folderName + folderInfo);
        checkBoxes.insert(folderName, checkBox);

        QCheckBox *statusCheckbox = new QCheckBox();
        statusCheckbox->setEnabled(false);
        checkBoxStatuses.insert(folderName, statusCheckbox);
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

QString Model::readApplicationShortNameFromFile(const QString& filePath) const {
    QStringList args;
    args << filePath;

    QProcess process;
    process.start(QCoreApplication::applicationDirPath() + "/" + "short_name.sh", args);
    if (!process.waitForStarted() || !process.waitForFinished()) {
        qWarning() << "Failed to execute script:" << process.errorString();
        return QString();
    }

    QString output = process.readAllStandardOutput();
    return output.trimmed();
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

bool Model::checkDebug(const QString& processName) const {
    QVector<int> ports = servicePorts.value(processName);

    if (ports.isEmpty()) {
        qDebug() << "No ports found for folder" << processName;
        return false;
    }

    for (int port : ports) {
        QTcpSocket socket;
        socket.connectToHost("127.0.0.1", port);
        if (socket.waitForConnected(1000)) {
            return true;
        }
    }

    return false;
}

QVector<int> Model::getServicePorts(const QString& folderName) const {
    QVector<int> ports;

    QString path = getDirectory()+ "/"+ folderName;
    QString prefix = shortNames.value(folderName);

    QStringList args;
    args << path << prefix;

    QProcess process;
    process.start(QCoreApplication::applicationDirPath() + "/" + "ports.sh", args);
    if (!process.waitForStarted() || !process.waitForFinished()) {
        qWarning() << "Failed to execute script:" << process.errorString();
        return ports;
    }

    QString output = process.readAllStandardOutput().trimmed();

    QStringList parts = output.split(" ");

    for (const QString& part : parts) {
        bool ok;
        int port = part.toInt(&ok);
        if (ok) ports.append(port);
    }

    return ports;
}

QString Model::getFolderInfo(const QString& folderName) const {
    QString shortName = shortNames.value(folderName);
    QVector<int> ports = servicePorts.value(folderName);

    QString portText;
    for (int i = 0; i < ports.size(); i++) {
        portText += QString::number(ports[i]);

        // Add "/" separator if it's not the last port
        if (i < ports.size() - 1) {
            portText += " / ";
        }
    }

    QString folderInfo;
    if (!shortName.isEmpty()) {
        folderInfo += " ("+ shortName +"):";
    }

    if (!folderInfo.isEmpty() && !portText.isEmpty()) {
        folderInfo += " ";
    }

    if (!portText.isEmpty()) {
        folderInfo += portText;
    }

    return folderInfo;
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

QString Model::getShortNameByName(const QString fileName) const {
    return shortNames.value(fileName);
}

bool Model::isServiceRunning(const QString& processName) const {
    int pid = getPidByName(processName);
    if (pid != -1) {
        return true;
    } else {
        return false;
    }
}

QMap<QString, QCheckBox*> Model::getCheckBoxStatuses() const {
    return checkBoxStatuses;
}

int Model::getProcessId(const QString& processName) const {
    int pid = getPidByName(processName);
    if (pid != -1) {
        return pid;
    }

    QVector<int> ports = servicePorts.value(processName);

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
