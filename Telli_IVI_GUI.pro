QT       += core gui multimedia multimediawidgets sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    account/accountselectpage.cpp \
    aircon_page.cpp \
    ambient_page.cpp \
    ambient_preview.cpp \
    db/db.cpp \
    headlamp_page.cpp \
    home_page.cpp \
    main.cpp \
    mainwindow.cpp \
    maincontainer.cpp \
    music_page.cpp \
    notificationbanner.cpp \
    ripplewidget.cpp \
    settings_page.cpp \
    trunk_page.cpp \
    update_handler.cpp \
    video_page.cpp \
    window_page.cpp \
    mapview.cpp \
    navi_page.cpp \
    navi_utils.cpp



HEADERS += \
    account/accountselectpage.h \
    account/random_compat.h \
    aircon_page.h \
    ambient_page.h \
    ambient_preview.h \
    db/db.h \
    headlamp_page.h \
    home_page.h \
    mainwindow.h \
    maincontainer.h \
    music_page.h \
    notificationbanner.h \
    ripplewidget.h \
    settings_page.h \
    trunk_page.h \
    update_handler.h \
    video_page.h \
    window_page.h \
    mapview.h \
    navi_page.h \
    navi_utils.h

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
