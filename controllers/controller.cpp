#include "controller.h"
#include "models/microservice_data.h"

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

            addCommand(name, command, args, excludedServices, buttonSize, executeForSelected, scriptName);

            settings.endGroup();
        }
    }
}

void Controller::refresh() {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->refreshCheckboxState();
    }
};

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

    QProcess process;
    QStringList args;
    args << commands.value(commandName)->getCommand()
         << model->getDirectory()
         << additionalArgs;

    qDebug() << "Starting script:" << scriptName << "with args:" << args;
    process.setProgram(QCoreApplication::applicationDirPath() + "/" + scriptName);
    process.setArguments(args);

    connect(&process, &QProcess::readyReadStandardOutput, [&process]() {
        qDebug() << "Output:" << process.readAllStandardOutput();
    });
    connect(&process, &QProcess::readyReadStandardError, [&process]() {
        qDebug() << "Error:" << process.readAllStandardError();
    });

    process.start();
    if (!process.waitForStarted()) {
        qDebug() << "Failed to start script:" << scriptName << "Error:" << process.errorString();
        return;
    }

    process.waitForFinished();

    if (process.exitCode() == 0) {
        qDebug() << "Command executed successfully.";
    } else {
        qDebug() << "Command failed with exit code:" << process.exitCode();
    }
}

int Controller::getCommandButtonSize(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        exit(EXIT_FAILURE);
    }

    return commands.value(commandName)->getButtonSize();
}

QStringList Controller::getCommandExcludedServices(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        exit(EXIT_FAILURE);
    }

    return commands.value(commandName)->getExcludedServices();
}

QStringList Controller::getCommandArgs(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        exit(EXIT_FAILURE);
    }

    return commands.value(commandName)->getArgs();
}

bool Controller::getCommandExecuteForSelected(const QString &commandName) const{
    if (!commands.contains(commandName)) {
        QMessageBox::critical(nullptr, "Error", "Command not found: " + commandName);
        exit(EXIT_FAILURE);
    }

    return commands.value(commandName)->getExecuteForSelected();
}

QMap<QString, Command*> Controller::getCommands() const {
    return commands;
}

void Controller::addCommand(const QString &name, const QString &command, const QStringList &args, const QStringList &excludedServices, const int buttonSize, const bool executeForSelected, const QString &scriptName) {
    Command *cmd = new Command(name, command, args, excludedServices, buttonSize, executeForSelected, scriptName);
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

void Controller::updateFlagStateForAllServices(const QString &flag, const Qt::CheckState state) {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->updateFlagState(flag, state);
    }
};
