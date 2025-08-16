#include "aircon_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "update_handler.h"
#include "system_status.h"

#include "notificationbanner.h"

#include <QMovie>

//#include <cstdio>             // std::
//using namespace std;

static QMovie* currentMovie = nullptr;

// connect
void initAirconConnections(MainWindow* w)
{
    /* Power btn */
    QObject::connect(w->getUi()->pushButton_aircon_on, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"aircon","command":"low","value":""})");
    });

    QObject::connect(w->getUi()->pushButton_aircon_off, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"aircon","command":"off","value":""})");
    });

    /* Speed btn */
    QObject::connect(w->getUi()->pushButton_aircon_low, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"aircon","command":"low","value":""})");
    });

    QObject::connect(w->getUi()->pushButton_aircon_medium, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"aircon","command":"mid","value":""})");
    });

    QObject::connect(w->getUi()->pushButton_aircon_high, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"aircon","command":"high","value":""})");
    });
}

// ON/OFF 버튼 처리
void handleAirconPowerButton(MainWindow* w, QPushButton* sender)
{
    auto ui = w->getUi();

    ui->pushButton_aircon_on->setText("");
    ui->pushButton_aircon_off->setText("");

    if (sender == ui->pushButton_aircon_on)
    {
        if (ui->pushButton_aircon_on->styleSheet().contains("ON_ON")) return;

        w->setAirconOn(true);

        ui->pushButton_aircon_on->setStyleSheet("border-image: url(:/images/ON_ON.png);");
        ui->pushButton_aircon_off->setStyleSheet("border-image: url(:/images/OFF_OFF.png);");

        handleAirconLevelButton(w, "LOW");  // 기본 LOW
    }
    else if (sender == ui->pushButton_aircon_off)
    {
        if (ui->pushButton_aircon_off->styleSheet().contains("OFF_ON")) return;

        w->setAirconOn(false);

        ui->pushButton_aircon_off->setStyleSheet("border-image: url(:/images/OFF_ON.png);");
        ui->pushButton_aircon_on->setStyleSheet("border-image: url(:/images/ON_OFF.png);");

        ui->pushButton_aircon_low->setStyleSheet("border-image: url(:/images/LOW_OFF.png);");
        ui->pushButton_aircon_medium->setStyleSheet("border-image: url(:/images/MEDIUM_OFF.png);");
        ui->pushButton_aircon_high->setStyleSheet("border-image: url(:/images/HIGH_OFF.png);");

        setAirconGif(w, ":/gif/aircon_fan.png", false);
    }
}

// 레벨 버튼 처리
void handleAirconLevelButton(MainWindow* w, const QString& level)
{
    w->getUi()->pushButton_aircon_low->setText("");
    w->getUi()->pushButton_aircon_medium->setText("");
    w->getUi()->pushButton_aircon_high->setText("");

    if (level == "LOW") {
        w->getUi()->pushButton_aircon_low->setStyleSheet("border-image: url(:/images/LOW_ON.png);");
        w->getUi()->pushButton_aircon_medium->setStyleSheet("border-image: url(:/images/MEDIUM_OFF.png);");
        w->getUi()->pushButton_aircon_high->setStyleSheet("border-image: url(:/images/HIGH_OFF.png);");

        setAirconGif(w, ":/gif/aircon_low_gif.gif", true);
    }
    else if (level == "MEDIUM") {
        w->getUi()->pushButton_aircon_low->setStyleSheet("border-image: url(:/images/LOW_OFF.png);");
        w->getUi()->pushButton_aircon_medium->setStyleSheet("border-image: url(:/images/MEDIUM_ON.png);");
        w->getUi()->pushButton_aircon_high->setStyleSheet("border-image: url(:/images/HIGH_OFF.png);");

        setAirconGif(w, ":/gif/aircon_medium_gif.gif", true);
    }
    else if (level == "HIGH") {
        w->getUi()->pushButton_aircon_low->setStyleSheet("border-image: url(:/images/LOW_OFF.png);");
        w->getUi()->pushButton_aircon_medium->setStyleSheet("border-image: url(:/images/MEDIUM_OFF.png);");
        w->getUi()->pushButton_aircon_high->setStyleSheet("border-image: url(:/images/HIGH_ON.png);");

        setAirconGif(w, ":/gif/aircon_high_gif.gif", true);
    }
    else {
        w->getUi()->pushButton_aircon_low->setStyleSheet("border-image: url(:/images/LOW_OFF.png);");
        w->getUi()->pushButton_aircon_medium->setStyleSheet("border-image: url(:/images/MEDIUM_OFF.png);");
        w->getUi()->pushButton_aircon_high->setStyleSheet("border-image: url(:/images/HIGH_OFF.png);");

        setAirconGif(w, ":/gif/aircon_fan.png", false);
    }
}

// GIF 표시 함수
void setAirconGif(MainWindow* w, const QString& path, bool isGif)
{
    QLabel* label1 = w->getUi()->aircon_gif;
    QLabel* label2 = w->getUi()->aircon_gif_2;

    // 기존 QMovie 제거
    if (currentMovie) {
        currentMovie->stop();
        delete currentMovie;
        currentMovie = nullptr;
    }

    if (isGif)
    {
        currentMovie = new QMovie(path);
        if (!currentMovie->isValid()) {
            qDebug() << "[ERROR] Invalid GIF:" << path;
            return;
        }

        label1->setMovie(currentMovie);
        label2->setMovie(currentMovie);

        label1->setScaledContents(true);
        label2->setScaledContents(true);

        label1->raise();
        label2->raise();

        label1->setVisible(true);
        label2->setVisible(true);

        currentMovie->start();
    }
    else
    {
        label1->setMovie(nullptr);
        label2->setMovie(nullptr);

        label1->setPixmap(QPixmap(path));
        label2->setPixmap(QPixmap(path));

        label1->setScaledContents(true);
        label2->setScaledContents(true);

        label1->raise();
        label2->raise();

        label1->setVisible(true);
        label2->setVisible(true);
    }
}
