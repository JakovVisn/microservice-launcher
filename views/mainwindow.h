#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QLineEdit>

#include "controllers/controller.h"

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
    void onRefreshButtonClicked();
    void onSaveActionClicked(const QString &saveName);
    void onSearchLineEditTextChanged();
    void onSearchLineEditEditingFinished();
    void onAddCommandClicked();
    void onAddSaveClicked();
    void onCustomButtonClicked(const QString &commandName);
    void onFlagControlPanelStateChanged(bool enabled);
    void onAddFlagClicked();
    void onApplyFlagToAllServices(const QString &flag);
    void onRemoveFlagFromAllServicesClicked(const QString &flag);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void readWindowSizeFromConfig();
    void loadSettings();
    void loadSavesFromConfigFile();
    void loadCommandsFromConfigFile();
    void loadMainWindowButtonsFromConfigFile();
    void saveCheckBoxStateToFile();
    void saveFlagsStateToFile();
    void updateServicesStatus();
    QStringList getCommandArguments(const QString &commandName);
    void executeForSelectedMicroservices(const QString &commandName, const QStringList &commandArgs);

    Ui::MainWindow *ui;
    Model* model;
    Controller* controller;

    QLineEdit *searchLineEdit;
    QVBoxLayout *contentLayout;

    QAction *saveCheckBox;
    QAction *showFlagControlPanelCheckBox;
    QVBoxLayout *mainLayout;
    QMenuBar *menuBar;
    QMenu *saveMenu;
    QMenu *commandMenu;
    QMenu *settingsMenu;
    QMenu *applyFlagsToAllServicesSubMenu;
    QMenu *removeFlagsFromAllServicesSubMenu;
    QLabel *servicesStatusLabel;

    int width;
    int height;
};
#endif // MAINWINDOW_H
