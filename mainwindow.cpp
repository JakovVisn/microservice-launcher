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

    setWindowTitle("Microservice Launcher (V1.0.0)");

    model = new Model();
    controller = new Controller(model);

    startButton = new QPushButton("Start", this);
    startButton->setFixedWidth(40);

    refreshButton = new QPushButton("Refresh", this);
    refreshButton->setFixedWidth(60);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshButtonClicked);

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

    buttonLayout1->setAlignment(Qt::AlignLeft);

    buttonLayout1->addWidget(startButton);
    buttonLayout1->addWidget(refreshButton);

    mainLayout->addLayout(buttonLayout1);
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

void MainWindow::onRefreshButtonClicked() {
    controller->refresh();
}
