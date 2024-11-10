#include "mainwindow.h"
#include "models/microservice_data.h"
#include "ui_mainwindow.h"

#include <QtWidgets/qpushbutton.h>
#include <QScrollArea>
#include <QSettings>
#include <QMessageBox>
#include <QDoubleValidator>
#include <QLabel>
#include <QKeyEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , servicesStatusLabel(new QLabel(this))
{
    ui->setupUi(this);

    setWindowTitle("Microservice Launcher (V2.1.0)");
    statusBar()->addPermanentWidget(servicesStatusLabel);

    model = new Model();
    controller = new Controller(model);

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Enter text to search");
    searchLineEdit->setFocusPolicy(Qt::ClickFocus);

    settingsMenu = new QMenu("Settings", this);
    saveMenu = new QMenu("Save", this);
    commandMenu = new QMenu("Additional Commands", this);

    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    menuBar->addMenu(settingsMenu);
    menuBar->addMenu(saveMenu);
    menuBar->addMenu(commandMenu);

    loadSavesFromConfigFile();
    loadCommandsFromConfigFile();
    loadSettings();

    connect(searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchLineEditTextChanged);
    connect(searchLineEdit, &QLineEdit::editingFinished, this, &MainWindow::onSearchLineEditEditingFinished);
    searchLineEdit->installEventFilter(this);

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveCheckBoxStateToFile);

    mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setSpacing(0);

    contentLayout = new QVBoxLayout;
    contentLayout->setAlignment(Qt::AlignTop);
    contentLayout->setSpacing(0);

    QWidget *scrollContent = new QWidget;
    scrollContent->setLayout(contentLayout);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scrollContent);

    QHBoxLayout *searchLayout = new QHBoxLayout;

    searchLayout->addWidget(searchLineEdit);

    loadMainWindowButtonsFromConfigFile();
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(scrollArea);

    QSettings settings(model->getSaveFile(), QSettings::IniFormat);
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        const QString &folderName = iter.key();

        if (saveCheckBox->isChecked()) {
            settings.beginGroup("CheckBoxState");
            bool isChecked = settings.value(folderName, false).toBool();
            iter.value()->setCheckBoxChecked(isChecked);
            settings.endGroup();
        }

        settings.beginGroup("Flag_"+iter.key());
        foreach (QString flag, model->getFlagNames()) {
            bool isChecked = settings.value(flag, false).toBool();
            iter.value()->addFlag(flag, showFlagControlPanelCheckBox->isChecked(), saveCheckBox->isChecked() ? isChecked : false);
        }

        settings.endGroup();

        QHBoxLayout *rowLayout = new QHBoxLayout;
        rowLayout->setAlignment(Qt::AlignLeft);
        rowLayout->setSpacing(10);
        rowLayout->addWidget(iter.value()->getStatusCheckBox());
        rowLayout->addWidget(iter.value()->getCheckBox());
        rowLayout->addWidget(iter.value()->getEnabledFlagsLabel());

        iter.value()->getCheckBox()->installEventFilter(this);

        iter.value()->getMicroserviceLayout()->addLayout(rowLayout);
        contentLayout->addLayout(iter.value()->getMicroserviceLayout());

        QHBoxLayout *flagsLayoutWithIndent = new QHBoxLayout;
        flagsLayoutWithIndent->addSpacing(35);
        flagsLayoutWithIndent->addLayout(iter.value()->getFlagsLayout());

        iter.value()->getMicroserviceLayout()->addLayout(flagsLayoutWithIndent);

        iter.value()->refreshCheckboxState();
        iter.value()->updateEnabledFlagsLabel();
    }

    readWindowSizeFromConfig();
    resize(width, height);
    updateServicesStatus();
}

void MainWindow::updateServicesStatus()
{
    int totalServices = model->getMicroservices().getDataMap().size();
    int activeServices = model->getMicroservices().getServicesByStatus(MicroserviceStatus::Active).size();
    int debugServices = model->getMicroservices().getServicesByStatus(MicroserviceStatus::Debug).size();

    int runningServices = activeServices + debugServices;

    servicesStatusLabel->setText(QString("Running services: %1/%2").arg(runningServices).arg(totalServices));
}

void MainWindow::onAddCommandClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Command");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    QLabel *nameLabel = new QLabel("Enter the name of the new command (Required):", &dialog);
    QLineEdit *nameLineEdit = new QLineEdit(&dialog);
    nameLineEdit->setPlaceholderText("My Command");
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameLineEdit);
    layout->addLayout(nameLayout);

    QHBoxLayout *scriptLayout = new QHBoxLayout;
    QLabel *scriptLabel = new QLabel("Enter the script name for the command (Required):", &dialog);
    QLineEdit *scriptLineEdit = new QLineEdit(&dialog);
    scriptLineEdit->setPlaceholderText("my_command.sh");
    scriptLayout->addWidget(scriptLabel);
    scriptLayout->addWidget(scriptLineEdit);
    layout->addLayout(scriptLayout);

    QHBoxLayout *commandLayout = new QHBoxLayout;
    QLabel *commandLabel = new QLabel("Enter the command (Optional):", &dialog);
    QLineEdit *commandLineEdit = new QLineEdit(&dialog);
    commandLineEdit->setPlaceholderText("make my-command");
    commandLayout->addWidget(commandLabel);
    commandLayout->addWidget(commandLineEdit);
    layout->addLayout(commandLayout);

    QHBoxLayout *argumentLayout = new QHBoxLayout;
    QLabel *argumentLabel = new QLabel("Enter arguments separated by comma (Optional):", &dialog);
    QLineEdit *argumentLineEdit = new QLineEdit(&dialog);
    argumentLineEdit->setPlaceholderText("arg1,arg2,arg3");
    argumentLayout->addWidget(argumentLabel);
    argumentLayout->addWidget(argumentLineEdit);
    layout->addLayout(argumentLayout);

    QCheckBox *executeForSelectedCheckBox = new QCheckBox("Execute for selected services", &dialog);
    layout->addWidget(executeForSelectedCheckBox);

    QCheckBox *disableSelectedServicesCheckBox = new QCheckBox("Disable currently selected services", &dialog);
    layout->addWidget(disableSelectedServicesCheckBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addWidget(&buttonBox);

    auto checkFields = [&](){
        bool fieldsFilled = !nameLineEdit->text().isEmpty() && !scriptLineEdit->text().isEmpty();
        buttonBox.button(QDialogButtonBox::Ok)->setEnabled(fieldsFilled);
    };

    connect(nameLineEdit, &QLineEdit::textChanged, this, checkFields);
    connect(scriptLineEdit, &QLineEdit::textChanged, this, checkFields);

    checkFields();

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString newCommandName = nameLineEdit->text();
    QString scriptName = scriptLineEdit->text();
    QString command = commandLineEdit->text();
    QStringList arguments = argumentLineEdit->text().split(',', Qt::SkipEmptyParts);
    bool executeForSelectedEnabled = executeForSelectedCheckBox->isChecked();
    bool disableSelectedServicesEnabled = disableSelectedServicesCheckBox->isChecked();

    if (disableSelectedServicesEnabled && !executeForSelectedEnabled) {
        QMessageBox::warning(this, "Warning", "Disabling currently selected services will only occur if 'Execute for selected services' is enabled.");
        return;
    }

    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    settings.beginGroup("Command_"+newCommandName);
    settings.setValue("scriptName", scriptName);
    settings.setValue("executeForSelected", executeForSelectedEnabled);

    if (!command.isEmpty()) {
        settings.setValue("command", command);
    }

    if (!arguments.isEmpty()) {
        settings.setValue("args", arguments);
    }

    QStringList checkedServicesNames;
    if (disableSelectedServicesEnabled) {
        QVector<MicroserviceData*> checkedServices = model->getMicroservices().getCheckedServices();
        if (!checkedServices.isEmpty()) {
            for (MicroserviceData* service : checkedServices) {
                checkedServicesNames << service->getName();
            }

            settings.setValue("excludedServices", checkedServicesNames);
        } else {
            QMessageBox::information(&dialog, "Info", "No service was checked, so this command will be applied to all services.");
        }
    }

    controller->addCommand(newCommandName, command, arguments, checkedServicesNames, "", executeForSelectedEnabled, scriptName);

    commandMenu->clear();
    loadCommandsFromConfigFile();
}

void MainWindow::onAddSaveClicked() {
    QDialog dialog(this);

    QVector<MicroserviceData*> checkedServices = model->getMicroservices().getCheckedServices();
    if (checkedServices.isEmpty()) {
        QMessageBox::warning(&dialog, "Warning", "At least one checkbox must be checked.");
        return;
    }

    dialog.setWindowTitle("Add New Save");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    QLabel *nameLabel = new QLabel("Enter the name of the new save (Required):", &dialog);
    QLineEdit *nameLineEdit = new QLineEdit(&dialog);
    nameLineEdit->setPlaceholderText("My Save");
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameLineEdit);
    layout->addLayout(nameLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addWidget(&buttonBox);

    auto checkFields = [&](){
        bool fieldFilled = !nameLineEdit->text().isEmpty();
        buttonBox.button(QDialogButtonBox::Ok)->setEnabled(fieldFilled);
    };

    connect(nameLineEdit, &QLineEdit::textChanged, this, checkFields);

    checkFields();

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString newSaveName = nameLineEdit->text();

    QStringList checkedServicesNames;
    for (MicroserviceData* service : checkedServices) {
        checkedServicesNames << service->getName();
    }

    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Save");
    settings.setValue(newSaveName, checkedServicesNames);
    settings.endGroup();

    saveMenu->clear();
    loadSavesFromConfigFile();
}

MainWindow::~MainWindow() {
    delete controller;
    delete model;
    delete ui;
}

void MainWindow::readWindowSizeFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("WindowSize");
    width = settings.value("width", 470).toInt();
    height = settings.value("height", 790).toInt();
    settings.endGroup();
}

void MainWindow::onSelectAllButtonClicked() {
    controller->selectAll();
}

void MainWindow::onDeselectAllButtonClicked() {
    controller->deselectAll();
}

void MainWindow::onRefreshButtonClicked() {
    controller->refresh();
    updateServicesStatus();
}

void MainWindow::loadSettings() {
    QSettings settings(model->getSaveFile(), QSettings::IniFormat);

    settings.beginGroup("SettingsState");
    bool isSaveChecked = settings.value("save", false).toBool();
    bool showFlagControlPanel = false;

    if (isSaveChecked) {
        showFlagControlPanel = settings.value("showFlagControlPanel", false).toBool();
    }

    QStringList flags;
    if (settings.contains("flags")) {
        flags = settings.value("flags").toStringList();
    }

    settings.endGroup();

    saveCheckBox = new QAction("Save State on Exit", this);
    saveCheckBox->setCheckable(true);
    saveCheckBox->setChecked(isSaveChecked);
    settingsMenu->addAction(saveCheckBox);

    QMenu *flagsSubMenu = new QMenu("Flags", this);

    showFlagControlPanelCheckBox = new QAction("Show Flag Control Panel", this);
    showFlagControlPanelCheckBox->setCheckable(true);
    showFlagControlPanelCheckBox->setChecked(showFlagControlPanel);
    connect(showFlagControlPanelCheckBox, &QAction::toggled, this, &MainWindow::onFlagControlPanelStateChanged);
    flagsSubMenu->addAction(showFlagControlPanelCheckBox);

    QAction *addFlagAction = new QAction("Add New Flag", this);
    connect(addFlagAction, &QAction::triggered, this, &MainWindow::onAddFlagClicked);
    flagsSubMenu->addAction(addFlagAction);

    applyFlagsToAllServicesSubMenu = new QMenu("Apply Flags to All Services", this);
    removeFlagsFromAllServicesSubMenu = new QMenu("Remove Flags from All Services", this);
    for (const QString& flag : flags) {
        QAction *applyFlagAction = new QAction(flag, this);
        connect(applyFlagAction, &QAction::triggered, this, [this, flag]() {
            onApplyFlagToAllServices(flag);
        });
        applyFlagsToAllServicesSubMenu->addAction(applyFlagAction);

        QAction *removeFlagAction = new QAction(flag, this);
        connect(removeFlagAction, &QAction::triggered, this, [this, flag]() {
            onRemoveFlagFromAllServicesClicked(flag);
        });
        removeFlagsFromAllServicesSubMenu->addAction(removeFlagAction);
    }

    flagsSubMenu->addMenu(applyFlagsToAllServicesSubMenu);
    flagsSubMenu->addMenu(removeFlagsFromAllServicesSubMenu);

    settingsMenu->addMenu(flagsSubMenu);

    QAction *addCommandAction = new QAction("Add New Command", this);
    connect(addCommandAction, &QAction::triggered, this, &MainWindow::onAddCommandClicked);
    settingsMenu->addAction(addCommandAction);

    QAction *addSaveAction = new QAction("Add New Save", this);
    connect(addSaveAction, &QAction::triggered, this, &MainWindow::onAddSaveClicked);
    settingsMenu->addAction(addSaveAction);
}

void MainWindow::loadSavesFromConfigFile() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Save");

    QStringList saveNames = settings.childKeys();
    for (const QString& saveName : saveNames) {
        QAction *save = new QAction(saveName, this);

        connect(save, &QAction::triggered, this, [this, saveName]() {
            onSaveActionClicked(saveName);
        });

        saveMenu->addAction(save);
    }

    settings.endGroup();
}

void MainWindow::loadCommandsFromConfigFile() {
    const QMap<QString, Command*>& commands = controller->getCommands();
    QMap<QString, Command*>::const_iterator iter;
    for (iter = commands.constBegin(); iter != commands.constEnd(); ++iter) {
        QString commandName = iter.value()->getName();
        QAction *action = new QAction(commandName, this);

        if (commandName == "Select All") {
            connect(action, &QAction::triggered, this, &MainWindow::onSelectAllButtonClicked);
        } else if (commandName == "Deselect All") {
            connect(action, &QAction::triggered, this, &MainWindow::onDeselectAllButtonClicked);
        } else if (commandName == "Refresh") {
            connect(action, &QAction::triggered, this, &MainWindow::onRefreshButtonClicked);
        } else {
            connect(action, &QAction::triggered, this, [this, commandName]() {
                onCustomButtonClicked(commandName);
            });
        }

        commandMenu->addAction(action);
    }
}

void MainWindow::loadMainWindowButtonsFromConfigFile() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    settings.beginGroup("MainWindowButtons");
    QStringList mainWindowButtonsGroups = settings.childKeys();
    for (const QString& group : mainWindowButtonsGroups) {
        QStringList commandNames = settings.value(group).toStringList();
        QHBoxLayout* groupLayout = new QHBoxLayout;
        groupLayout->setAlignment(Qt::AlignLeft);
        for (const QString& commandName : commandNames) {
            QPushButton *pushButton = new QPushButton(commandName, this);

            QString style = controller->getCommandButtonStyle(commandName);
            if (!style.isEmpty()) {
                pushButton->setStyleSheet(style);
            }

            if (commandName == "Select All") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onSelectAllButtonClicked);
            } else if (commandName == "Deselect All") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onDeselectAllButtonClicked);
            } else if (commandName == "Refresh") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);
            } else {
                connect(pushButton, &QPushButton::clicked, this, [this, commandName]() {
                    onCustomButtonClicked(commandName);
                });
            }

            groupLayout->addWidget(pushButton);
        }

        mainLayout->addLayout(groupLayout);
    }

    settings.endGroup();
}

void MainWindow::onCustomButtonClicked(const QString &commandName) {
    QStringList commandArgs;

    if (!controller->getCommandArgs(commandName).isEmpty()) {
        commandArgs = getCommandArguments(commandName);
        if (commandArgs.isEmpty()) {
            return; // Exit if dialog was closed without accepting
        }
    }

    if (!controller->getCommandExecuteForSelected(commandName)) {
        controller->executeScript(commandName, commandArgs);
    } else {
        executeForSelectedMicroservices(commandName, commandArgs);
    }
}

QStringList MainWindow::getCommandArguments(const QString &commandName) {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Enter Command Arguments"));
    dialog.setMinimumWidth(dialog.windowTitle().size() * 12);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QStringList commandArgumentNames = controller->getCommandArgs(commandName);

    for (const QString &argument : commandArgumentNames) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        lineEdit->setPlaceholderText(argument);
        layout->addWidget(lineEdit);
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addWidget(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return QStringList(); // Return empty list if dialog is closed
    }

    QStringList args;
    foreach (QObject *obj, dialog.children()) {
        if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj)) {
            args << lineEdit->text();
        }
    }

    return args;
}

void MainWindow::executeForSelectedMicroservices(const QString &commandName, const QStringList &commandArgs) {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    for (auto iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        if (!iter.value()->getCheckBox()->isChecked()) {
            continue; // Skip if checkbox is not checked
        }

        QString processName = iter.key();
        if (controller->getCommandExcludedServices(commandName).contains(processName)) {
            continue; // Skip excluded services
        }

        iter.value()->refreshCheckboxState();
        QString pid = iter.value()->getPIDByPorts();

        QStringList args;
        args << processName
             << iter.value()->getShortName()
             << pid
             << commandArgs
             << iter.value()->getEnabledFlags();

        controller->executeScript(commandName, args);
    }
}

void MainWindow::onSaveActionClicked(const QString &saveName) {
    controller->selectDetermined(saveName);
}

void MainWindow::onSearchLineEditTextChanged() {
    QString searchText = searchLineEdit->text();
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    for (auto iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value()->getCheckBox();
        QCheckBox* statusCheckBox = iter.value()->getStatusCheckBox();
        bool matchesSearch = searchText.isEmpty() || checkBox->text().contains(searchText, Qt::CaseInsensitive);
        checkBox->setVisible(matchesSearch);
        statusCheckBox->setVisible(matchesSearch);

        if (showFlagControlPanelCheckBox->isChecked()) {
            iter.value()->setFlagsVisible(matchesSearch);
        }

        contentLayout->removeItem(iter.value()->getMicroserviceLayout());
        if (matchesSearch) {
            contentLayout->addLayout(iter.value()->getMicroserviceLayout());
        }

        iter.value()->updateEnabledFlagsLabel();
    }
}

void MainWindow::onSearchLineEditEditingFinished() {
    QList<QCheckBox*> visibleCheckBoxes;
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    for (auto iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value()->getCheckBox();
        if (checkBox->isVisible()) {
            visibleCheckBoxes.append(checkBox);
        }
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget* clickedWidget = qobject_cast<QWidget*>(obj);
        QCheckBox* clickedCheckBox = qobject_cast<QCheckBox*>(clickedWidget);

        if (clickedCheckBox) {
            clickedCheckBox->setChecked(!clickedCheckBox->isChecked());
            clickedCheckBox->clearFocus();
            searchLineEdit->clear();
            onSearchLineEditTextChanged();
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QWidget* currentFocus = focusWidget();
            QCheckBox* focusedCheckBox = qobject_cast<QCheckBox*>(currentFocus);

            if (focusedCheckBox) {
                focusedCheckBox->setChecked(!focusedCheckBox->isChecked());
                searchLineEdit->clear();
                onSearchLineEditTextChanged();
                searchLineEdit->setFocus();
                return true;
            }

            QCheckBox* firstVisibleCheckBox = nullptr;
            QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
            for (auto iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
                QCheckBox* checkBox = iter.value()->getCheckBox();
                if (checkBox->isVisible()) {
                    firstVisibleCheckBox = checkBox;
                    break;
                }
            }

            if (firstVisibleCheckBox && !searchLineEdit->text().isEmpty()) {
                firstVisibleCheckBox->setChecked(!firstVisibleCheckBox->isChecked());
                searchLineEdit->clear();
                onSearchLineEditTextChanged();
                searchLineEdit->setFocus();
            }

            return true;
        } else if (keyEvent->key() == Qt::Key_Tab) {
            QList<QCheckBox*> visibleCheckBoxes;
            QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
            for (auto iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
                QCheckBox* checkBox = iter.value()->getCheckBox();
                if (checkBox->isVisible()) {
                    checkBox->setFocusPolicy(Qt::StrongFocus);
                    visibleCheckBoxes.append(checkBox);
                }
            }

            QWidget* currentFocus = focusWidget();
            int index = visibleCheckBoxes.indexOf(qobject_cast<QCheckBox*>(currentFocus));

            if (index != -1 && index < visibleCheckBoxes.size() - 1) {
                visibleCheckBoxes.at(index + 1)->setFocus();
            } else if (index == -1 && !visibleCheckBoxes.isEmpty()) {
                visibleCheckBoxes.first()->setFocus();
            } else if (index == visibleCheckBoxes.size() - 1) {
                visibleCheckBoxes.first()->setFocus();
            }

            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::saveCheckBoxStateToFile() {
    QSettings settings(model->getSaveFile(), QSettings::IniFormat);
    settings.beginGroup("CheckBoxState");

    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        QString folderName = iter.key();
        bool isChecked = iter.value()->getCheckBox()->isChecked();
        settings.setValue(folderName, isChecked);
    }

    settings.endGroup();

    settings.beginGroup("SettingsState");
    settings.setValue("save", saveCheckBox->isChecked());
    settings.setValue("showFlagControlPanel", showFlagControlPanelCheckBox->isChecked());

    settings.endGroup();

    saveFlagsStateToFile();
}

void MainWindow::saveFlagsStateToFile() {
    QSettings settings(model->getSaveFile(), QSettings::IniFormat);

    settings.beginGroup("SettingsState");
    settings.setValue("flags", model->getFlagNames());
    settings.endGroup();

    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        settings.beginGroup("Flag_" + iter.key());

        foreach (QCheckBox *checkBox, iter.value()->getFlagCheckBoxes()) {
            settings.setValue(checkBox->text(), checkBox->isChecked());
        }

        settings.endGroup();
    }
}

void MainWindow::onFlagControlPanelStateChanged(bool enabled) {
    QMap<QString, MicroserviceData*> microservicesMap = model->getMicroservices().getDataMap();
    QMap<QString, MicroserviceData*>::const_iterator iter;
    for (iter = microservicesMap.constBegin(); iter != microservicesMap.constEnd(); ++iter) {
        iter.value()->setFlagsVisible(enabled);
    }
}

void MainWindow::onAddFlagClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Flag");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QHBoxLayout *flagLayout = new QHBoxLayout;
    QLabel *flagLabel = new QLabel("Enter the new flag (Required):", &dialog);
    QLineEdit *flagLineEdit = new QLineEdit(&dialog);
    flagLineEdit->setPlaceholderText("My Flag");
    flagLayout->addWidget(flagLabel);
    flagLayout->addWidget(flagLineEdit);
    layout->addLayout(flagLayout);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addWidget(&buttonBox);

    auto checkFields = [&](){
        bool fieldsFilled = !flagLineEdit->text().isEmpty();
        buttonBox.button(QDialogButtonBox::Ok)->setEnabled(fieldsFilled);
    };

    connect(flagLineEdit, &QLineEdit::textChanged, this, checkFields);

    checkFields();

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString newFlag = flagLineEdit->text();

    model->addFlagName(newFlag);
    controller->addFlag(newFlag, showFlagControlPanelCheckBox->isChecked());

    QAction *applyFlagAction = new QAction(newFlag, this);
    connect(applyFlagAction, &QAction::triggered, this, [this, newFlag]() {
        onApplyFlagToAllServices(newFlag);
    });
    applyFlagsToAllServicesSubMenu->addAction(applyFlagAction);

    QAction *removeFlagAction = new QAction(newFlag, this);
    connect(removeFlagAction, &QAction::triggered, this, [this, newFlag]() {
        onRemoveFlagFromAllServicesClicked(newFlag);
    });
    removeFlagsFromAllServicesSubMenu->addAction(removeFlagAction);
}

void MainWindow::onApplyFlagToAllServices(const QString &flag) {
    controller->updateFlagStateForAllServices(flag, Qt::Checked);
}

void MainWindow::onRemoveFlagFromAllServicesClicked(const QString &flag) {
    controller->updateFlagStateForAllServices(flag, Qt::Unchecked);
}
