#include "microservice_data.h"
#include "model.h"

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QTcpSocket>

MicroserviceData::MicroserviceData(const QString name, const QString directory)
    : name(name)
    , directory(directory)
    , shortName(readApplicationShortNameFromFile(directory + "/" + name))
    , ports(readPortsFromFile())
{
    QString folderInfo = getFolderInfo();
    checkBox = new QCheckBox(name + folderInfo);

    statusCheckBox = new QCheckBox();
    statusCheckBox->setEnabled(false);
}

QString MicroserviceData::readApplicationShortNameFromFile(const QString& filePath) const {
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

QVector<int> MicroserviceData::readPortsFromFile() const {
    QVector<int> ports;

    QString path = directory + "/"+ name;
    QString prefix = shortName;

    QStringList args;
    args << path << prefix << QCoreApplication::applicationDirPath();

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

QString MicroserviceData::getFolderInfo() const {
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

int MicroserviceData::getPid() const {
    QString cmd = "pgrep -x " + name;
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

bool MicroserviceData::checkDebug() const {
    if (ports.isEmpty()) {
        qDebug() << "No ports found for folder" << name;
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

bool MicroserviceData::isServiceRunning() const {
    int pid = getPid();
    if (pid != -1) {
        return true;
    }

    return false;
}

void MicroserviceData::refreshCheckboxState() {
    bool isRunning = isServiceRunning();
    bool isDebug = isRunning ? false : checkDebug();

    statusCheckBox->setChecked(isRunning ? isRunning : isDebug);
    statusCheckBox->setStyleSheet(isDebug ? "background-color: green;" : "");
    status = isRunning ? MicroserviceStatus::Active : isDebug ? MicroserviceStatus::Debug : MicroserviceStatus::Inactive;
}

MicroserviceStatus MicroserviceData::getStatus() const {
    return status;
}

QString MicroserviceData::getShortName() const {
    return shortName;
}

QString MicroserviceData::getName() const {
    return name;
}

void MicroserviceData::setCheckBoxChecked(bool checked) {
    checkBox->setChecked(checked);
}

QCheckBox* MicroserviceData::getCheckBox() {
    return checkBox;
}

QCheckBox* MicroserviceData::getstatusCheckBox() {
    return statusCheckBox;
}

QVector<int> MicroserviceData::getPorts() const {
    return ports;
}
