#include "microservice_data.h"

#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QTcpSocket>
#include <QDir>

MicroserviceData::MicroserviceData(const QString name, const QString directory)
    : name(name)
    , shortName(readApplicationShortNameFromFile(QDir(directory).filePath(name)))
    , ports(readPortsFromFile(directory))
    , flagsLayout(new QHBoxLayout)
{
    flagsLayout->setAlignment(Qt::AlignLeft);
    flagsLayout->setSpacing(10);

    QString folderInfo = getFolderInfo();
    checkBox = new QCheckBox(name + folderInfo);

    statusCheckBox = new QCheckBox();
    statusCheckBox->setEnabled(false);

    enabledFlagsLabel = new QLabel(getEnabledFlags().join(", "));
    enabledFlagsLabel->setStyleSheet("color: #7161d4;");
}

QString MicroserviceData::readApplicationShortNameFromFile(const QString& filePath) const {
    QStringList args;
    args << filePath;

    QProcess process;
    QString scriptPath = QDir(QCoreApplication::applicationDirPath()).filePath("short_name.sh");
    process.start(scriptPath, args);
    if (!process.waitForStarted() || !process.waitForFinished()) {
        qWarning() << "Failed to execute script:" << process.errorString();
        return QString();
    }

    QString output = process.readAllStandardOutput();
    return output.trimmed();
}

QVector<int> MicroserviceData::readPortsFromFile(const QString directory) const {
    QVector<int> ports;

    QStringList args;
    args << QDir(directory).filePath(name)
         << shortName;

    QProcess process;
    QString scriptPath = QDir(QCoreApplication::applicationDirPath()).filePath("ports.sh");
    process.start(scriptPath, args);
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

    if (ports.isEmpty()) {
        qDebug() << output;
    }

    return ports;
}

QString MicroserviceData::getFolderInfo() const {
    QStringList portStrings;
    for (const auto &port : ports) {
        portStrings << QString::number(port);
    }

    QString portText = portStrings.join(" / ");

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
    QProcess process;
    QStringList arguments;

    #if defined(Q_OS_LINUX)
        arguments << "-C" << name << "-o" << "pid=";
        process.start("ps", arguments);
    #elif defined(Q_OS_MACOS)
        arguments << "-x" << name;
        process.start("pgrep", arguments);
    #else
        qWarning() << "Platform not supported";
        return -1;
    #endif

    if (!process.waitForStarted()) {
        qWarning() << "Failed to start process";
        return -1;
    }

    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit) {
        qWarning() << "Process did not exit normally";
        return -1;
    }

    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        return -1; // Return -1 if process is not found
    }

    bool ok;
    int pid = output.toInt(&ok);
    if (!ok) {
        qWarning() << "Failed to convert output to PID";
        return -1;
    }

    return pid;
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
    return getPid() == -1 ? false : true;
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

QCheckBox* MicroserviceData::getStatusCheckBox() {
    return statusCheckBox;
}

QVector<int> MicroserviceData::getPorts() const {
    return ports;
}

void MicroserviceData::setFlagsVisible(bool visible){
    foreach (QCheckBox *checkBox, flagCheckBoxes) {
        if (visible) {
            flagsLayout->addWidget(checkBox);
            checkBox->show();
        } else {
            flagsLayout->removeWidget(checkBox);
            checkBox->hide();
        }
    }
}

QHBoxLayout* MicroserviceData::getFlagsLayout() const {
    return flagsLayout;
}

void MicroserviceData::addFlag(const QString flag, bool visible, bool check) {
    QCheckBox *flagCheckBox = new QCheckBox(flag);
    flagCheckBox->setChecked(check);
    flagCheckBoxes.append(flagCheckBox);

    if (visible) {
        flagsLayout->addWidget(flagCheckBox);
    }

    QObject *context = new QObject(flagCheckBox);

    flagCheckBox->connect(flagCheckBox, &QCheckBox::stateChanged, context, [this]() {
        updateEnabledFlagsLabel();
    });
}

QStringList MicroserviceData::getEnabledFlags() const {
    QStringList enabledFlags;
    foreach (QCheckBox *checkBox, flagCheckBoxes) {
        if (checkBox->isChecked()) {
            enabledFlags << checkBox->text();
        }
    }

    return enabledFlags;
}

QVector<QCheckBox*> MicroserviceData::getFlagCheckBoxes() const {
    return flagCheckBoxes;
}

void MicroserviceData::updateEnabledFlagsLabel() {
    QStringList enabledFlags = getEnabledFlags();
    QString enabledFlagsText = enabledFlags.join(", ");
    enabledFlagsLabel->setText(enabledFlagsText);
}

QLabel* MicroserviceData::getEnabledFlagsLabel() const {
    return enabledFlagsLabel;
}

void MicroserviceData::updateFlagState(const QString flag, const Qt::CheckState state) const {
    foreach (QCheckBox *checkBox, flagCheckBoxes) {
        if (checkBox->text() == flag) {
            checkBox->setCheckState(state);
        }
    }
}

QString MicroserviceData::getPIDByPorts() const {
    QVector<int> ports = getPorts();
    if (ports.isEmpty()) {
        qDebug() << "No ports found for folder" << name;
        return "";
    }

    for (int port : ports) {
        QString cmd = "lsof -i :" + QString::number(port) + " -t";
        QProcess process;

        process.start("/usr/bin/env", QStringList() << "bash" << "-c" << cmd);
        process.waitForFinished();

        QString result = process.readAllStandardOutput().trimmed();
        if (!result.isEmpty()) {
            qDebug() << "Found PID for port" << port << ":" << result;
            return result;
        } else {
            qDebug() << "No PID found for port" << port;
        }
    }

    return ""; // Return empty if no process is found on any port
}
