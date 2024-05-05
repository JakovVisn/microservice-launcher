#include "controller.h"

#include <csignal>

#include <QtCore/qprocess.h>
#include <QSettings>
#include <QCoreApplication>

Controller::Controller(Model *model_): model(model_) {
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

void Controller::start(const QString& processName) {
    refreshCheckboxState(processName);
    QMap<QString, QCheckBox*> checkBoxStatuses = model->getCheckBoxStatuses();
    QCheckBox* statusCheckbox = checkBoxStatuses.value(processName);
    if (statusCheckbox->isChecked()) {
        qDebug() << "Process" << processName << "is already running or in debug mode.";
        return;
    }

    QStringList args;
    args << processName << model->getShortNameByName(processName);

    executeScript("Start", args);
}

void Controller::stop(const QString& processName) {
    qDebug() << "Trying to stop service:" << processName;
    stopProcess(processName);
};

void Controller::refreshCheckboxState(const QString& processName) {
    QMap<QString, QCheckBox*> checkBoxStatuses = model->getCheckBoxStatuses();
    QCheckBox* statusCheckbox = checkBoxStatuses.value(processName);

    bool isRunning = model->isServiceRunning(processName);
    bool isDebug = isRunning ? false : model->checkDebug(processName);

    statusCheckbox->setChecked(isRunning ? isRunning : isDebug);
    statusCheckbox->setStyleSheet(isDebug ? "background-color: green;" : "");
};

void Controller::refresh() {
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        refreshCheckboxState(iter.key());
    }
};

void Controller::stopProcess(const QString& processName) const {
    int processId = model->getProcessId(processName);
    if (processId == -1) {
        qDebug() << "Process" << processName << "not found. Cannot stop.";
        return;
    }

    ::kill(processId, SIGINT);
    qDebug() << "Sent SIGINT to stop process" << processName;
}

void Controller::selectAll() {
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value();
        checkBox->setChecked(true);
    }
};

void Controller::deselectAll() {
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value();
        checkBox->setChecked(false);
    }
};

void Controller::selectDetermined(const QString &actionName) {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Action");
    QStringList checkboxNames = settings.value(actionName).toStringList();
    settings.endGroup();

    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    for (const QString &checkboxName : checkboxNames) {
        QCheckBox *checkBox = checkBoxes.value(checkboxName);
        if (checkBox) {
            checkBox->setChecked(true);
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
