#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "model.h"

class Controller: public QWidget
{
    Q_OBJECT
public:
    explicit Controller(Model* model);

    void start(const QString& processName);
    void refresh();
    void refreshCheckboxState(const QString& processName);

private:
    Model* model;
    QMap<QString, float> delays;
    QMap<QString, QString> commands;

    void loadDelaysFromConfig();
    void loadCommandsFromConfig();
};

#endif // CONTROLLER_H
