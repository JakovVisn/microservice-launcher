#include "controller.h"
#include "models/microservice_data.h"
#include "models/microservice_status.h"

#include <csignal>

#include <QtCore/qprocess.h>
#include <QSettings>
#include <QCoreApplication>
#include <QMessageBox>

Controller::Controller(Model *model)
    : model(model)
{
    loadCommandsFromConfig();
}

void Controller::loadCommandsFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    foreach (const QString &group, settings.childGroups()) {
        if (group.startsWith("Command_")) {
            settings.beginGroup(group);

            QString name = group.mid(QString("Command_").length());
            QString command = settings.value("command").toString();
            float delay = settings.value("delay").toFloat();
            int buttonSize = settings.value("buttonSize").toInt();
            bool executeForSelected = settings.value("executeForSelected").toBool();;
            QString scriptName = settings.value("scriptName").toString();

            QStringList excludedServices;
            if (settings.contains("excludedServices")) {
                excludedServices = settings.value("excludedServices").toStringList();
            }

            QStringList args;
            if (settings.contains("args")) {
                args = settings.value("args").toStringList();
            }

            addCommand(name, command, args, delay, excludedServices, buttonSize, executeForSelected, scriptName);

            settings.endGroup();
        }
    }
}

void Controller::start(MicroserviceData* microservice) {
    microservice->refreshCheckboxState();
    if (microservice->getStatus() != MicroserviceStatus::Inactive) {
        qDebug() << "Process" << microservice->getName() << "is already running or in debug mode.";
        return;
    }

    QStringList args;
    args << microservice->getName() << microservice->getShortName() << microservice->getEnabledFlags();

    executeScript("Start", args);
}

void Controller::stop(const QString& processName) {
    qDebug() << "Trying to stop service:" << processName;
    stopProcess(processName);
};

void Controller::refresh() {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->refreshCheckboxState();
    }
};

void Controller::stopProcess(const QString& processName) const {
    int processID = model->getProcessID(processName);
    if (processID == -1) {
        qDebug() << "Process" << processName << "not found. Cannot stop.";
        return;
    }

    ::kill(processID, SIGINT);
    qDebug() << "Sent SIGINT to stop process" << processName;
}

void Controller::selectAll() {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->setCheckBoxChecked(true);
    }
};

void Controller::deselectAll() {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->setCheckBoxChecked(false);
    }
};

void Controller::selectDetermined(const QString &actionName) {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Action");
    QStringList checkboxNames = settings.value(actionName).toStringList();
    settings.endGroup();

    MicroserviceDataMap microservices = model->getMicroservices();
    for (const QString &checkboxName : checkboxNames) {
        if (microservices.contains(checkboxName)) {
            microservices.value(checkboxName)->setCheckBoxChecked(true);
        }
    }
}

void Controller::executeScript(const QString &commandName, const QStringList &additionalArgs) {
    QString scriptName = commands.value(commandName)->getScriptName();
    QString command = commands.value(commandName)->getCommand();
    QString newTabDelay = QString::number(model->getNewTabDelay());
    QString customDelay = QString::number(commands.value(commandName)->getDelay());
    QString workingDirectory = model->getDirectory();

    QProcess process;
    QStringList args;
    args << command << newTabDelay << customDelay << workingDirectory << additionalArgs;
    qDebug() << "args:" << args;
    process.start(QCoreApplication::applicationDirPath() + "/" + scriptName, args);
    process.waitForFinished();

    if (process.exitCode() == 0) {
        qDebug() << "Command executed successfully.";
    } else {
        QString errorOutput = process.readAllStandardError();
        qDebug() << "Error executing shell script:" << errorOutput;
    }
}

int Controller::getCommandButtonSize(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        QCoreApplication::quit();
    }

    return commands.value(commandName)->getButtonSize();
}

QStringList Controller::getCommandExcludedServices(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        QCoreApplication::quit();
    }

    return commands.value(commandName)->getExcludedServices();
}

QStringList Controller::getCommandArgs(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        QCoreApplication::quit();
    }

    return commands.value(commandName)->getArgs();
}

bool Controller::getCommandExecuteForSelected(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        QCoreApplication::quit();
    }

    return commands.value(commandName)->getExecuteForSelected();
}

QMap<QString, Command*> Controller::getCommands() const {
    return commands;
}

void Controller::addCommand(const QString &name, const QString &command, const QStringList &args, const float delay, const QStringList &excludedServices, const int buttonSize, const bool executeForSelected, const QString &scriptName) {
    Command *cmd = new Command(name, command, args, delay, excludedServices, buttonSize, executeForSelected, scriptName);
    commands.insert(name, cmd);
}

void Controller::addFlag(const QString &flag, bool visible) {
    if (flag.isEmpty()) {
        return;
    }

    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->addFlag(flag, visible);
    }
}
