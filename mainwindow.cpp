#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QScrollArea>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Microservice Launcher (V1.4.0)");

    model = new Model();
    controller = new Controller(model);

    selectAllButton = new QPushButton("Select All", this);
    selectAllButton->setFixedWidth(85);

    deselectAllButton = new QPushButton("Deselect All", this);
    deselectAllButton->setFixedWidth(85);

    startButton = new QPushButton("Start", this);
    startButton->setFixedWidth(40);

    stopButton = new QPushButton("Stop", this);
    stopButton->setFixedWidth(40);

    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setFixedWidth(60);

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
    QHBoxLayout *buttonLayout2 = new QHBoxLayout;
    QHBoxLayout *searchLayout = new QHBoxLayout;

    buttonLayout1->setAlignment(Qt::AlignLeft);
    buttonLayout2->setAlignment(Qt::AlignLeft);

    buttonLayout1->addWidget(selectAllButton);
    buttonLayout2->addWidget(deselectAllButton);
    buttonLayout1->addWidget(startButton);
    buttonLayout2->addWidget(stopButton);
    buttonLayout1->addWidget(refreshButton);

    searchLayout->addWidget(searchLineEdit);

    mainLayout->addLayout(buttonLayout1);
    mainLayout->addLayout(buttonLayout2);
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(scrollArea);

    QMap<QString, QCheckBox*> checkBoxes = model->getCheckBoxes();
    QMap<QString, QCheckBox*> checkBoxStatuses = model->getCheckBoxStatuses();
    QMap<QString, QCheckBox*>::const_iterator iter;
    for (iter = checkBoxes.constBegin(); iter != checkBoxes.constEnd(); ++iter) {
        const QString &folderName = iter.key();

        QHBoxLayout *rowLayout = new QHBoxLayout;
        rowLayout->setAlignment(Qt::AlignLeft);
        rowLayout->setSpacing(10);
        rowLayout->addWidget(checkBoxStatuses.value(folderName));
        rowLayout->addWidget(iter.value());
        contentLayout->addLayout(rowLayout);

        controller->refreshCheckboxState(folderName);
    }

    readWindowSizeFromConfig();
    resize(width, height);

    loadActionsFromConfigFile();
}

MainWindow::~MainWindow() {
    delete controller;
    delete model;
    delete ui;
}

void MainWindow::readWindowSizeFromConfig() {
    QSettings settings(model->getConfigFile(), QSettings::IniFormat);
    settings.beginGroup("WindowSize");
    width = settings.value("width", 455).toInt();
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
    QMenu *fileMenu = new QMenu("Save", this);

    for (const QString& key : keys) {
        QString actionName = settings.value(key).toString();
        QAction *action = new QAction(actionName, this);

        connect(action, &QAction::triggered, this, [this, actionName]() {
            onSaveActionClicked(actionName);
        });

        fileMenu->addAction(action);
    }

    settings.endGroup();

    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->addMenu(fileMenu);
    setMenuBar(menuBar);
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
