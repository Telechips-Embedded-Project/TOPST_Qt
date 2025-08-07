QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    account/accountmanager.cpp \
    account/accountselectpage.cpp \
    aircon_page.cpp \
    ambient_page.cpp \
    enter.cpp \
    headlamp_page.cpp \
    main.cpp \
    mainwindow.cpp \
    maincontainer.cpp \
    ripplewidget.cpp \
    sensor_handler.cpp \
    trunk_page.cpp \
    video_page.cpp \
    window_page.cpp

HEADERS += \
    account/accountmanager.h \
    account/accountselectpage.h \
    account/accountsettings.h \
    aircon_page.h \
    ambient_page.h \
    enter.h \
    headlamp_page.h \
    mainwindow.h \
    maincontainer.h \
    ripplewidget.h \
    sensor_handler.h \
    trunk_page.h \
    video_page.h \
    window_page.h

FORMS += \
    account/accountselectpage.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

DISTFILES += \
    account/account_daddy.json
