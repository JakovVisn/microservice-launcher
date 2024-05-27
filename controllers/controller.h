#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "models/model.h"

class Controller: public QWidget
{
    Q_OBJECT
public:
    explicit Controller(Model* model);

    void selectAll();
    void deselectAll();
    void start(MicroserviceData* microservice);
    void stop(const QString& processName);
    void refresh();
    void selectDetermined(const QString &actionName);
    void executeScript(const QString &commandName, const QStringList &additionalArgs = QStringList());
    void setDelay(const QString &commandName, const float &delay);
    void setCommand(const QString &commandName, const QString &command);

private:
    Model* model;
    QMap<QString, float> delays;
    QMap<QString, QString> commands;

    void loadDelaysFromConfig();
    void loadCommandsFromConfig();
    void stopProcess(const QString& processName) const;
};

#endif // CONTROLLER_H
