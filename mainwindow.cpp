#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QScrollArea>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDoubleValidator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Microservice Launcher (V1.7.4)");

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
    loadDisabledServicesFromConfig();
    loadSettings();

    connect(searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchLineEditTextChanged);
    connect(searchLineEdit, &QLineEdit::editingFinished, this, &MainWindow::onSearchLineEditEditingFinished);

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveCheckBoxStateToFile);

    mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setSpacing(0);

    QVBoxLayout *contentLayout = new QVBoxLayout;
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
    settings.beginGroup("CheckBoxState");
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*> checkBoxStatuses = model->getCheckBoxStatuses();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        const QString &folderName = iter.key();

        if (saveCheckBox->isChecked()) {
            bool isChecked = settings.value(folderName, false).toBool();
            iter.value()->setChecked(isChecked);
        }

        QHBoxLayout *rowLayout = new QHBoxLayout;
        rowLayout->setAlignment(Qt::AlignLeft);
        rowLayout->setSpacing(10);
        rowLayout->addWidget(checkBoxStatuses.value(folderName));
        rowLayout->addWidget(iter.value());
        contentLayout->addLayout(rowLayout);

        controller->refreshCheckboxState(folderName);
    }

    settings.endGroup();

    readWindowSizeFromConfig();
    resize(width, height);
}

void MainWindow::onAddCommandClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Command");

    QFormLayout formLayout(&dialog);

    QLineEdit *nameLineEdit = new QLineEdit(&dialog);
    formLayout.addRow("Enter the name of the new command:", nameLineEdit);

    QLineEdit *scriptLineEdit = new QLineEdit(&dialog);
    formLayout.addRow("Enter the script name for the command:", scriptLineEdit);

    QLineEdit *commandLineEdit = new QLineEdit(&dialog);
    commandLineEdit->setPlaceholderText("Can be empty");
    formLayout.addRow("Enter the command:", commandLineEdit);

    QLineEdit *delayLineEdit = new QLineEdit(&dialog);
    QDoubleValidator *validator = new QDoubleValidator(delayLineEdit);
    validator->setBottom(0.0);
    delayLineEdit->setValidator(validator);
    delayLineEdit->setPlaceholderText("Can be empty");
    formLayout.addRow("Enter the delay for the command:", delayLineEdit);

    QCheckBox *executeForSelectedCheckBox = new QCheckBox("Execute for selected services", &dialog);
    formLayout.addRow(executeForSelectedCheckBox);

    QCheckBox *disableSelectedServicesCheckBox = new QCheckBox("Disable currently selected services", &dialog);
    formLayout.addRow(disableSelectedServicesCheckBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    formLayout.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString newCommandName = nameLineEdit->text();
    QString scriptName = scriptLineEdit->text();
    QString command = commandLineEdit->text();
    QString delay = delayLineEdit->text();
    bool executeForSelectedEnabled = executeForSelectedCheckBox->isChecked();
    bool disableSelectedServicesEnabled = disableSelectedServicesCheckBox->isChecked();

    if (newCommandName.isEmpty()) {
        return;
    }

    if (scriptName.isEmpty()) {
        return;
    }

    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    settings.beginGroup("AdditionalCommands");
    QString newCommandKey = "Command" + QString::number(settings.childKeys().count() + 1);
    settings.setValue(newCommandKey, newCommandName);
    settings.endGroup();

    settings.beginGroup("ScriptNames");
    settings.setValue(newCommandName, scriptName);
    settings.endGroup();

    settings.beginGroup("ExecuteForSelected");
    settings.setValue(newCommandName, executeForSelectedEnabled);
    settings.endGroup();

    if (!delay.isEmpty()){
        settings.beginGroup("Delays");
        settings.setValue(newCommandName, delay);
        settings.endGroup();

        controller->setDelay(newCommandName, delay.toFloat());
    }

    if (!command.isEmpty()) {
        settings.beginGroup("Commands");
        settings.setValue(newCommandName, command);
        settings.endGroup();

        controller->setCommand(newCommandName, command);
    }

    if (disableSelectedServicesEnabled && executeForSelectedEnabled) {
        settings.beginGroup("DisabledServicesForCommands");
        QStringList checkedServices;
        QMap<QString, QCheckBox*> checkedCheckBoxes = getCheckedCheckBoxes();
        if (!checkedCheckBoxes.isEmpty()) {
            QMap<QString, QCheckBox*>::const_iterator iter;
            for (iter = checkedCheckBoxes.constBegin(); iter != checkedCheckBoxes.constEnd(); ++iter) {
                checkedServices << iter.key();
            }

            settings.setValue(newCommandName, checkedServices);
            disabledServicesForCommands[newCommandName] = checkedServices;
        }

        settings.endGroup();
    }

    commandMenu->clear();
    loadCommandsFromConfigFile();
}

void MainWindow::onAddSaveClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Save");

    QFormLayout formLayout(&dialog);

    QLineEdit *nameLineEdit = new QLineEdit(&dialog);
    formLayout.addRow("Enter the name of the new save:", nameLineEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    formLayout.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QString newSaveName = nameLineEdit->text();

    if (newSaveName.isEmpty()) {
        return;
    }

    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    settings.beginGroup("Action");
    QStringList checkedServices;
    QMap<QString, QCheckBox*> checkedCheckBoxes = getCheckedCheckBoxes();
    if (checkedCheckBoxes.isEmpty()) {
        return;
    }

    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkedCheckBoxes.constBegin(); iter != checkedCheckBoxes.constEnd(); ++iter) {
        checkedServices << iter.key();
    }

    settings.setValue(newSaveName, checkedServices);


    settings.endGroup();

    settings.beginGroup("Save");
    QString newSaveKey = "action" + QString::number(settings.childKeys().count() + 1);
    settings.setValue(newSaveKey, newSaveName);
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

void MainWindow::onStartButtonClicked() {
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value();
        if (checkBox->isChecked()) {
            QString folderName = iter.key();
            qDebug() << "Trying to launch service:" << folderName;

            controller->start(folderName);
        }
    }
}

void MainWindow::onStopButtonClicked() {
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value();
        if (checkBox->isChecked()) {
            QString processName = iter.key();
            controller->stop(processName);
        }
    }
}

void MainWindow::onRefreshButtonClicked() {
    controller->refresh();
}

void MainWindow::loadSettings() {
    QSettings settings(model->getSaveFile(), QSettings::IniFormat);

    settings.beginGroup("CheckBoxSaveState");
    bool isSaveChecked = settings.value("save", false).toBool();
    settings.endGroup();

    saveCheckBox = new QAction("Save&&Exit", this);
    saveCheckBox->setCheckable(true);
    saveCheckBox->setChecked(isSaveChecked);

    settingsMenu->addAction(saveCheckBox);

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
    QStringList keys = settings.childKeys();

    for (const QString& key : keys) {
        QString actionName = settings.value(key).toString();
        QAction *action = new QAction(actionName, this);

        connect(action, &QAction::triggered, this, [this, actionName]() {
            onSaveActionClicked(actionName);
        });

        saveMenu->addAction(action);
    }

    settings.endGroup();
}

void MainWindow::loadCommandsFromConfigFile() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("AdditionalCommands");
    QStringList commands = settings.childKeys();

    for (const QString& key : commands) {
        QString commandName = settings.value(key).toString();
        QAction *action = new QAction(commandName, this);

        connect(action, &QAction::triggered, this, [this, commandName]() {
            onCustomButtonClicked(commandName);
        });

        commandMenu->addAction(action);
    }

    settings.endGroup();
}

void MainWindow::loadMainWindowButtonsFromConfigFile() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    QMap<QString, int> buttonSize;
    settings.beginGroup("ButtonSize");
    QStringList keys = settings.childKeys();

    for (const QString& key : keys) {
        int size = settings.value(key).toInt();
        buttonSize.insert(key, size);
    }

    settings.endGroup();

    settings.beginGroup("MainWindowButtons");
    QStringList mainWindowButtonsGroups = settings.childKeys();
    for (const QString& group : mainWindowButtonsGroups) {
        QStringList commandNames = settings.value(group).toStringList();
        QHBoxLayout* groupLayout = new QHBoxLayout;
        groupLayout->setAlignment(Qt::AlignLeft);
        for (const QString& commandName : commandNames) {
            QPushButton *pushButton = new QPushButton(commandName, this);

            int size  = buttonSize.value(commandName);
            if (size != 0) {
                pushButton->setFixedWidth(size);
            }

            if (commandName == "Select All") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onSelectAllButtonClicked);
            } else if (commandName == "Deselect All") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onDeselectAllButtonClicked);
            } else if (commandName == "Start") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
            } else if (commandName == "Stop") {
                connect(pushButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
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
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("ExecuteForSelected");
    bool executeForSelectedEnabled = settings.value(commandName, false).toBool();
    settings.endGroup();

    if (!executeForSelectedEnabled) {
        controller->executeScript(commandName);
    } else {
        QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
        QMap<QString, QCheckBox*>::const_iterator iter;
        for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
            QCheckBox* checkBox = iter.value();
            if (!checkBox->isChecked()) {
                continue;
            }

            QString processName = iter.key();
            if (disabledServicesForCommands[commandName].contains(processName)) {
                continue;
            }

            QStringList args;
            args << processName << model->getShortNameByName(processName);

            controller->executeScript(commandName, args);
        }
    }
}

void MainWindow::onSaveActionClicked(const QString &actionName) {
    controller->selectDetermined(actionName);
}

void MainWindow::onSearchLineEditTextChanged() {
    QString searchText = searchLineEdit->text();
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;

    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        QCheckBox* checkBox = iter.value();
        if (searchText.isEmpty()) {
            checkBox->setStyleSheet("");
        } else if (checkBox->text().contains(searchText, Qt::CaseInsensitive)) {
            checkBox->setStyleSheet("color: red;");
        } else {
            checkBox->setStyleSheet("");
        }
    }
}

void MainWindow::onSearchLineEditEditingFinished() {
    searchLineEdit->clear();
    onSearchLineEditTextChanged();
}

void MainWindow::saveCheckBoxStateToFile() {
    QSettings settings(model->getSaveFile(), QSettings::IniFormat);
    settings.beginGroup("CheckBoxState");

    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        QString folderName = iter.key();
        bool isChecked = iter.value()->isChecked();
        settings.setValue(folderName, isChecked);
    }

    settings.endGroup();

    settings.beginGroup("CheckBoxSaveState");
    settings.setValue("save", saveCheckBox->isChecked());

    settings.endGroup();
}

void MainWindow::loadDisabledServicesFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    settings.beginGroup("DisabledServicesForCommands");
    QStringList keys = settings.childKeys();
    foreach (const QString &key, keys) {
        QStringList services = settings.value(key).toStringList();
        disabledServicesForCommands[key] = services;
    }

    settings.endGroup();
}

QMap<QString, QCheckBox*> MainWindow::getCheckedCheckBoxes() {
    QMap<QString, QCheckBox*> allCheckBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*> checkedCheckBoxes;

    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = allCheckBoxes.constBegin(); iter != allCheckBoxes.constEnd(); ++iter) {
        const QString& checkBoxName = iter.key();
        QCheckBox* checkBox = iter.value();

        if (checkBox->isChecked()) {
            checkedCheckBoxes.insert(checkBoxName, checkBox);
        }
    }

    return checkedCheckBoxes;
}
