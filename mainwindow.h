#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
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
    void onAddCommandClicked();
    void onAddSaveClicked();
    void onCustomButtonClicked(const QString &commandName);

private:
    void readWindowSizeFromConfig();
    void loadSettings();
    void loadSavesFromConfigFile();
    void loadCommandsFromConfigFile();
    void loadMainWindowButtonsFromConfigFile();
    void saveCheckBoxStateToFile();
    void loadDisabledServicesFromConfig();
    QMap<QString, QCheckBox*> getCheckedCheckBoxes();

    Ui::MainWindow *ui;
    Model* model;
    Controller* controller;

    QLineEdit *searchLineEdit;

    QAction *saveCheckBox;
    QVBoxLayout *mainLayout;
    QMenuBar *menuBar;
    QMenu *saveMenu;
    QMenu *commandMenu;
    QMenu *settingsMenu;

    QMap<QString, QStringList> disabledServicesForCommands;

    int width;
    int height;
};
#endif // MAINWINDOW_H
