#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "controller.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void readWindowSizeFromConfig();

    Ui::MainWindow *ui;
    Model* model;
    Controller* controller;

    int width;
    int height;
};
#endif // MAINWINDOW_H
