#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QScrollArea>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Microservice Launcher (V1.8.0)");

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
    loadCommandArgumentsFromConfig();
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

    QHBoxLayout *delayLayout = new QHBoxLayout;
    QLabel *delayLabel = new QLabel("Enter the delay for the command (Optional):", &dialog);
    QLineEdit *delayLineEdit = new QLineEdit(&dialog);
    QDoubleValidator *validator = new QDoubleValidator(0.0, 999999.0, 2, delayLineEdit);
    delayLineEdit->setValidator(validator);
    delayLineEdit->setPlaceholderText("5,0");
    delayLayout->addWidget(delayLabel);
    delayLayout->addWidget(delayLineEdit);
    layout->addLayout(delayLayout);

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
    QString delay = delayLineEdit->text();
    QStringList arguments = argumentLineEdit->text().split(',', Qt::SkipEmptyParts);
    bool executeForSelectedEnabled = executeForSelectedCheckBox->isChecked();
    bool disableSelectedServicesEnabled = disableSelectedServicesCheckBox->isChecked();

    if (disableSelectedServicesEnabled && !executeForSelectedEnabled) {
        QMessageBox::warning(this, "Warning", "Disabling currently selected services will only occur if 'Execute for selected services' is enabled.");
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

    if (disableSelectedServicesEnabled) {
        QStringList checkedServices;
        QMap<QString, QCheckBox*> checkedCheckBoxes = getCheckedCheckBoxes();
        if (!checkedCheckBoxes.isEmpty()) {
            QMap<QString, QCheckBox*>::const_iterator iter;
            for (iter = checkedCheckBoxes.constBegin(); iter != checkedCheckBoxes.constEnd(); ++iter) {
                checkedServices << iter.key();
            }

            disabledServicesForCommands[newCommandName] = checkedServices;
            settings.beginGroup("DisabledServicesForCommands");
            settings.setValue(newCommandName, checkedServices);
            settings.endGroup();
        } else {
            QMessageBox::information(&dialog, "Info", "No service was checked, so this command will be applied to all services.");
        }
    }

    if (!arguments.isEmpty()) {
        commandArguments[newCommandName] = arguments;

        settings.beginGroup("CommandArguments");
        settings.setValue(newCommandName, arguments);
        settings.endGroup();
    }

    commandMenu->clear();
    loadCommandsFromConfigFile();
}

void MainWindow::onAddSaveClicked() {
    QDialog dialog(this);

    QMap<QString, QCheckBox*> checkedCheckBoxes = getCheckedCheckBoxes();
    if (checkedCheckBoxes.isEmpty()) {
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

    QStringList checkedServices;
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkedCheckBoxes.constBegin(); iter != checkedCheckBoxes.constEnd(); ++iter) {
        checkedServices << iter.key();
    }

    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Action");
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
    QStringList commandArgs;
    if (!commandArguments[commandName].isEmpty()) {
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Enter Command Arguments"));
        dialog.setMinimumWidth(dialog.windowTitle().size()*12);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QStringList commandArgumentNames = commandArguments[commandName];
        foreach (const QString &argument, commandArgumentNames) {
            QLineEdit *lineEdit = new QLineEdit(&dialog);
            lineEdit->setPlaceholderText(argument);
            layout->addWidget(lineEdit);
        }

        QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
        layout->addWidget(&buttonBox);

        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() != QDialog::Accepted) {
            return;
        }

        foreach (QObject *obj, dialog.children()) {
            QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj);
            if (lineEdit) {
                commandArgs << lineEdit->text();
            }
        }
    }

    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("ExecuteForSelected");
    bool executeForSelectedEnabled = settings.value(commandName, false).toBool();
    settings.endGroup();

    if (!executeForSelectedEnabled) {
        controller->executeScript(commandName, commandArgs);
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
            args << processName << model->getShortNameByName(processName) << commandArgs;

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

void MainWindow::loadCommandArgumentsFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);

    settings.beginGroup("CommandArguments");
    QStringList keys = settings.childKeys();
    foreach (const QString &key, keys) {
        QStringList arguments = settings.value(key).toStringList();
        commandArguments[key] = arguments;
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
