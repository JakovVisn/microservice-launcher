#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
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
    void onSelectAllButtonClicked();
    void onDeselectAllButtonClicked();
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onRefreshButtonClicked();
    void onSaveActionClicked(const QString &actionName);
    void onSearchLineEditTextChanged();
    void onSearchLineEditEditingFinished();

private:
    void readWindowSizeFromConfig();
    void loadActionsFromConfigFile();

    Ui::MainWindow *ui;
    Model* model;
    Controller* controller;

    QPushButton *selectAllButton;
    QPushButton *deselectAllButton;
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *refreshButton;

    QLineEdit *searchLineEdit;

    int width;
    int height;
};
#endif // MAINWINDOW_H
