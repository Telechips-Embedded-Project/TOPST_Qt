#include "ambient_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

static QString currentColor = "";

// 초기 버튼 연결
void initAmbientConnections(MainWindow* w)
{
    // ON/OFF 버튼
    QObject::connect(w->getUi()->pushButton_ambient_on, &QPushButton::clicked, [=]() {
        handleAmbientPowerButton(w, w->getUi()->pushButton_ambient_on);
    });

    QObject::connect(w->getUi()->pushButton_ambient_off, &QPushButton::clicked, [=]() {
        handleAmbientPowerButton(w, w->getUi()->pushButton_ambient_off);
    });

    // 색상 버튼
    QObject::connect(w->getUi()->pushButton_ambient_red, &QPushButton::clicked, [=]() {
        if (w->isAmbientOn()) handleAmbientColorButton(w, "red");
    });

    QObject::connect(w->getUi()->pushButton_ambient_yellow, &QPushButton::clicked, [=]() {
        if (w->isAmbientOn()) handleAmbientColorButton(w, "yellow");
    });

    QObject::connect(w->getUi()->pushButton_ambient_green, &QPushButton::clicked, [=]() {
        if (w->isAmbientOn()) handleAmbientColorButton(w, "green");
    });

    QObject::connect(w->getUi()->pushButton_ambient_rainbow, &QPushButton::clicked, [=]() {
        if (w->isAmbientOn()) handleAmbientColorButton(w, "rainbow");
    });
}

// ON/OFF 버튼 처리
void handleAmbientPowerButton(MainWindow* w, QPushButton* sender)
{
    auto ui = w->getUi();

    // Remove text from all buttons
    ui->pushButton_ambient_on->setText("");
    ui->pushButton_ambient_off->setText("");

    if (sender == ui->pushButton_ambient_on)
    {
        if (ui->pushButton_ambient_on->styleSheet().contains("AMBIENT_ON_ON")) return;

        w->setAmbientOn(true);  // 상태 저장

        // 이미지 변경
        ui->pushButton_ambient_on->setStyleSheet("border-image: url(:/images/AMBIENT_ON_ON.png);");
        ui->pushButton_ambient_off->setStyleSheet("border-image: url(:/images/AMBIENT_OFF_OFF.png);");

        // RED 기본 선택
        handleAmbientColorButton(w, "red");
    }
    else if (sender == ui->pushButton_ambient_off)
    {
        if (ui->pushButton_ambient_off->styleSheet().contains("AMBIENT_OFF_ON")) return;

        w->setAmbientOn(false);  // 상태 저장

        ui->pushButton_ambient_off->setStyleSheet("border-image: url(:/images/AMBIENT_OFF_ON.png);");
        ui->pushButton_ambient_on->setStyleSheet("border-image: url(:/images/AMBIENT_ON_OFF.png);");

        // 모든 색상 OFF
        ui->pushButton_ambient_red->setStyleSheet("border-image: url(:/images/AMBIENT_RED_OFF.png);");
        ui->pushButton_ambient_yellow->setStyleSheet("border-image: url(:/images/AMBIENT_YELLOW_OFF.png);");
        ui->pushButton_ambient_green->setStyleSheet("border-image: url(:/images/AMBIENT_GREEN_OFF.png);");
        ui->pushButton_ambient_rainbow->setStyleSheet("border-image: url(:/images/AMBIENT_RAINBOW_OFF.png);");

        currentColor = "";
    }
}

// 색상 버튼 처리
void handleAmbientColorButton(MainWindow* w, const QString& color)
{
    auto ui = w->getUi();

    // Remove text from all buttons
    ui->pushButton_ambient_red->setText("");
    ui->pushButton_ambient_yellow->setText("");
    ui->pushButton_ambient_green->setText("");
    ui->pushButton_ambient_rainbow->setText("");

    // 모든 색상 OFF
    ui->pushButton_ambient_red->setStyleSheet("border-image: url(:/images/AMBIENT_RED_OFF.png);");
    ui->pushButton_ambient_yellow->setStyleSheet("border-image: url(:/images/AMBIENT_YELLOW_OFF.png);");
    ui->pushButton_ambient_green->setStyleSheet("border-image: url(:/images/AMBIENT_GREEN_OFF.png);");
    ui->pushButton_ambient_rainbow->setStyleSheet("border-image: url(:/images/AMBIENT_RAINBOW_OFF.png);");

    // 선택된 색상 ON
    if (color == "red")
        ui->pushButton_ambient_red->setStyleSheet("border-image: url(:/images/AMBIENT_RED_ON.png);");
    else if (color == "yellow")
        ui->pushButton_ambient_yellow->setStyleSheet("border-image: url(:/images/AMBIENT_YELLOW_ON.png);");
    else if (color == "green")
        ui->pushButton_ambient_green->setStyleSheet("border-image: url(:/images/AMBIENT_GREEN_ON.png);");
    else if (color == "rainbow")
        ui->pushButton_ambient_rainbow->setStyleSheet("border-image: url(:/images/AMBIENT_RAINBOW_ON.png);");

    currentColor = color;
}
