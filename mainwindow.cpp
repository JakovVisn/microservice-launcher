#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
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

    setWindowTitle("Microservice Launcher (V1.6.0)");

    model = new Model();
    controller = new Controller(model);

    selectAllButton = new QPushButton("Select All", this);
    deselectAllButton = new QPushButton("Deselect All", this);
    startButton = new QPushButton("Start", this);
    stopButton = new QPushButton("Stop", this);
    refreshButton = new QPushButton("Refresh", this);

    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Enter text to search");
    searchLineEdit->setFocusPolicy(Qt::ClickFocus);

    connect(selectAllButton, &QPushButton::clicked, this, &MainWindow::onSelectAllButtonClicked);
    connect(deselectAllButton, &QPushButton::clicked, this, &MainWindow::onDeselectAllButtonClicked);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);
    connect(searchLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchLineEditTextChanged);
    connect(searchLineEdit, &QLineEdit::editingFinished, this, &MainWindow::onSearchLineEditEditingFinished);

    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveCheckBoxStateToFile);

    QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);
    mainLayout->setSpacing(0);

    QVBoxLayout *contentLayout = new QVBoxLayout;
    contentLayout->setAlignment(Qt::AlignTop);
    contentLayout->setSpacing(0);

    QWidget *scrollContent = new QWidget;
    scrollContent->setLayout(contentLayout);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(scrollContent);

    QHBoxLayout *buttonLayout1 = new QHBoxLayout;
    QHBoxLayout *searchLayout = new QHBoxLayout;

    buttonLayout1->setAlignment(Qt::AlignLeft);

    buttonLayout1->addWidget(selectAllButton);
    buttonLayout1->addWidget(deselectAllButton);
    buttonLayout1->addWidget(startButton);
    buttonLayout1->addWidget(stopButton);
    buttonLayout1->addWidget(refreshButton);

    searchLayout->addWidget(searchLineEdit);

    QSettings settings(model->getSaveFile(), QSettings::IniFormat);

    settings.beginGroup("CheckBoxSaveState");
    bool isSaveChecked = settings.value("save", false).toBool();
    model->getSaveCheckBox().setChecked(isSaveChecked);
    buttonLayout1->addWidget(&model->getSaveCheckBox());

    settings.endGroup();

    mainLayout->addLayout(buttonLayout1);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(scrollArea);

    settings.beginGroup("CheckBoxState");
    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*> checkBoxStatuses = model->getCheckBoxStatuses();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        const QString &folderName = iter.key();

        if (isSaveChecked) {
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

    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    fileMenu = new QMenu("Save", this);
    commandMenu = new QMenu("Additional Commands", this);

    loadActionsFromConfigFile();
    loadCommandsFromConfigFile();
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

    commandMenu->clear();
    loadCommandsFromConfigFile();
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

void MainWindow::loadActionsFromConfigFile() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("Save");
    QStringList keys = settings.childKeys();

    for (const QString& key : keys) {
        QString actionName = settings.value(key).toString();
        QAction *action = new QAction(actionName, this);

        connect(action, &QAction::triggered, this, [this, actionName]() {
            onSaveActionClicked(actionName);
        });

        fileMenu->addAction(action);
    }

    settings.endGroup();

    menuBar->addMenu(fileMenu);
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

    QAction *addCommandAction = new QAction("Add New Command", this);
    connect(addCommandAction, &QAction::triggered, this, &MainWindow::onAddCommandClicked);
    commandMenu->addAction(addCommandAction);

    menuBar->addMenu(commandMenu);
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
            if (checkBox->isChecked()) {
                QString processName = iter.key();
                QStringList args;
                args << processName << model->getShortNameByName(processName);

                controller->executeScript(commandName, args);
            }
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
    settings.setValue("save", model->getSaveCheckBox().isChecked());

    settings.endGroup();
}
