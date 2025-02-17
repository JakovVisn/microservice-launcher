#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "models/model.h"
#include "command.h"

class Controller: public QWidget
{
    Q_OBJECT
public:
    explicit Controller(Model* model);

    void selectAll();
    void deselectAll();
    void refresh();
    void selectDetermined(const QString &saveName);
    void executeScript(const QString &commandName, const QStringList &additionalArgs = QStringList());
    QString getCommandButtonStyle(const QString &commandName) const;
    bool getCommandExecuteForSelected(const QString &commandName) const;
    QStringList getCommandExcludedServices(const QString &commandName) const;
    QStringList getCommandArgs(const QString &commandName) const;
    QMap<QString, Command*> getCommands() const;
    void addCommand(
        const QString &name,
        const QString &command,
        const QStringList &args,
        const QStringList &excludedServices,
        const QString buttonStyle,
        const bool executeForSelected,
        const QString &scriptName);
    void addFlag(const QString &flag, bool visible);
    void updateFlagStateForAllServices(const QString &flag, Qt::CheckState);

private:
    Model* model;

    void loadCommandsFromConfig();
    QMap<QString, Command*> commands;
};

#endif // CONTROLLER_H
