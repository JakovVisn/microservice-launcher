#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "model.h"

class Controller: public QWidget
{
    Q_OBJECT
public:
    explicit Controller(Model* model);

private:
    Model* model;
};

#endif // CONTROLLER_H
