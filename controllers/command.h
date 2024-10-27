#ifndef COMMAND_H
#define COMMAND_H

#include <QStringList>

class Command {
public:
    explicit Command(const QString &name, const QString &command, const QStringList &args, const QStringList &excludedServices, const int buttonSize, const bool executeForSelected, const QString &scriptName);
    QString getName() const;
    QString getCommand() const;
    int getButtonSize() const;
    QString getScriptName() const;
    QStringList getExcludedServices() const;
    QStringList getArgs() const;
    bool getExecuteForSelected() const;

private:
    QString name;
    QString command;
    QStringList args;
    QStringList excludedServices;
    int buttonSize;
    bool executeForSelected;
    QString scriptName;
};

#endif // COMMAND_H
