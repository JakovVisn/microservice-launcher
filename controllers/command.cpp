#include "command.h"

Command::Command(
    const QString &name,
    const QString &command,
    const QStringList &args,
    const QStringList &excludedServices,
    const QString buttonStyle,
    const bool executeForSelected,
    const QString &scriptName)
    : name(name)
    , command(command)
    , args(args)
    , excludedServices(excludedServices)
    , buttonStyle(buttonStyle)
    , executeForSelected(executeForSelected)
    , scriptName(scriptName)
{}

QString Command::getName() const {
    return name;
}

QString Command::getCommand() const {
    return command;
}

QString Command::getButtonStyle() const {
    return buttonStyle;
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
