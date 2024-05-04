#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets/qpushbutton.h>

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

private slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onRefreshButtonClicked();

private:
    void readWindowSizeFromConfig();

    Ui::MainWindow *ui;
    Model* model;
    Controller* controller;

    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *refreshButton;

    int width;
    int height;
};
#endif // MAINWINDOW_H
