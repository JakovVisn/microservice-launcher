QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    views/mainwindow.cpp \
    controllers/controller.cpp \
    models/model.cpp \
    models/microservice_data.cpp \
    models/microservice_data_map.cpp \
    controllers/command.cpp

HEADERS += \
    views/mainwindow.h \
    controllers/controller.h \
    models/model.h \
    models/microservice_data.h \
    models/microservice_status.h \
    models/microservice_data_map.h \
    controllers/command.h

FORMS += \
    views/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
