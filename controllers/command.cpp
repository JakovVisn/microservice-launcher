#include "command.h"

Command::Command(const QString &name, const QString &command, const QStringList &args, const QStringList &excludedServices, const int buttonSize, const bool executeForSelected, const QString &scriptName)
    : name(name)
    , command(command)
    , args(args)
    , excludedServices(excludedServices)
    , buttonSize(buttonSize)
    , executeForSelected(executeForSelected)
    , scriptName(scriptName)
{}

QString Command::getName() const {
    return name;
}

QString Command::getCommand() const {
    return command;
}

int Command::getButtonSize() const {
    return buttonSize;
}

QString Command::getScriptName() const {
    return scriptName;
}

QStringList Command::getExcludedServices() const {
    return excludedServices;
}

QStringList Command::getArgs() const {
    return args;
}

bool Command::getExecuteForSelected() const {
    return executeForSelected;
}
