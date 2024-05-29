#include "controller.h"
#include "models/microservice_data.h"
#include "models/microservice_status.h"

#include <csignal>

#include <QtCore/qprocess.h>
#include <QSettings>
#include <QCoreApplication>

Controller::Controller(Model *model)
    : model(model)
{
    loadDelaysFromConfig();
    loadCommandsFromConfig();
}

void Controller::loadDelaysFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Delays");
    QStringList keys = settings.childKeys();

    for (const QString& key : keys) {
        float delay = settings.value(key).toFloat();
        delays.insert(key, delay);
        qDebug() << "key: " << key << " delay: " << delay;
    }

    settings.endGroup();
}

void Controller::loadCommandsFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Commands");
    QStringList keys = settings.childKeys();

    for (const QString& key : keys) {
        QString command = settings.value(key).toString();
        commands.insert(key, command);
    }

    settings.endGroup();
}

void Controller::start(MicroserviceData* microservice) {
    microservice->refreshCheckboxState();
    if (microservice->getStatus() != MicroserviceStatus::Inactive) {
        qDebug() << "Process" << microservice->getName() << "is already running or in debug mode.";
        return;
    }

    QStringList args;
    args << microservice->getName() << microservice->getShortName();

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
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("ScriptNames");
    QString scriptName = settings.value(commandName).toString();
    settings.endGroup();

    QString command = commands.value(commandName);
    QString newTabDelay = QString::number(delays.value("newTab", 0));
    QString customDelay = QString::number(delays.value(commandName));
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

void Controller::setDelay(const QString &commandName, const float &delay){
    delays.insert(commandName, delay);
}

void Controller::setCommand(const QString &commandName, const QString &command){
    commands.insert(commandName, command);
}
