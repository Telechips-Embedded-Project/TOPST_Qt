#include "ambient_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "update_handler.h"
#include "system_status.h"

#include <cstring>
//#include <cstdio>             // std::

//using namespace std;

static QString currentColor = "";

static QString g_lastAmbientColor = "red";
QString ambientGetLastColor() { return g_lastAmbientColor; }
void    ambientSetLastColor(const QString& c) { g_lastAmbientColor = c; }

void initAmbientConnections(MainWindow* w)
{
    auto ui = w->getUi();

    // On/Off btn
    QObject::connect(ui->pushButton_ambient_on, &QPushButton::clicked, [=]()
    {   
        QString chosen = g_lastAmbientColor;

        if (chosen.isEmpty() || chosen == "off") {
            if (shm_ptr) {
                QString c = QString::fromLatin1(shm_ptr->ambient.color)
                                .trimmed().toLower();
                if (!c.isEmpty() && c != "off") chosen = c;
            }
        }
        if (chosen.isEmpty() || chosen == "off") chosen = "red";

        w->sendJsonToFifo(
            QString(R"({"device":"ambient","command":"color","value":"%1"})")
                .arg(chosen)
        );
    });

    QObject::connect(ui->pushButton_ambient_off, &QPushButton::clicked, [=]()
    {
        w->sendJsonToFifo(R"({"device":"ambient","command":"color","value":"off"})");
    });

    // color btn
    QObject::connect(ui->pushButton_ambient_red, &QPushButton::clicked, [=]()
    {
        g_lastAmbientColor = "red";
        w->sendJsonToFifo(R"({"device":"ambient","command":"color","value":"red"})");
    });

    QObject::connect(ui->pushButton_ambient_yellow, &QPushButton::clicked, [=]()
    {
        g_lastAmbientColor = "yellow";
        w->sendJsonToFifo(R"({"device":"ambient","command":"color","value":"yellow"})");
    });

    QObject::connect(ui->pushButton_ambient_green, &QPushButton::clicked, [=]()
    {
        g_lastAmbientColor = "green";
        w->sendJsonToFifo(R"({"device":"ambient","command":"color","value":"green"})");
    });

    QObject::connect(ui->pushButton_ambient_rainbow, &QPushButton::clicked, [=]()
    {
        g_lastAmbientColor = "rainbow";
        w->sendJsonToFifo(R"({"device":"ambient","command":"color","value":"rainbow"})");
    });

    QObject::connect(ui->pushButton_ambient_low, &QPushButton::clicked, [=]()
    {
        w->sendJsonToFifo(R"({"device":"ambient","command":"brightness","value":"low"})");
    });

    QObject::connect(ui->pushButton_ambient_medium, &QPushButton::clicked, [=]()
    {
        w->sendJsonToFifo(R"({"device":"ambient","command":"brightness","value":"mid"})");
    });

    QObject::connect(ui->pushButton_ambient_high, &QPushButton::clicked, [=]()
    {
        w->sendJsonToFifo(R"({"device":"ambient","command":"brightness","value":"high"})");
    });
}

void handleAmbientPowerButton(MainWindow* w, QPushButton* sender)
{
    auto ui = w->getUi();
    ui->pushButton_ambient_on->setText("");
    ui->pushButton_ambient_off->setText("");

    if (sender == ui->pushButton_ambient_on) {
        if (ui->pushButton_ambient_on->styleSheet().contains("AMBIENT_ON_ON")) return;
        ui->pushButton_ambient_on->setStyleSheet("border-image: url(:/images/AMBIENT_ON_ON.png);");
        ui->pushButton_ambient_off->setStyleSheet("border-image: url(:/images/AMBIENT_OFF_OFF.png);");
    } else {
        if (ui->pushButton_ambient_off->styleSheet().contains("AMBIENT_OFF_ON")) return;
        ui->pushButton_ambient_off->setStyleSheet("border-image: url(:/images/AMBIENT_OFF_ON.png);");
        ui->pushButton_ambient_on->setStyleSheet("border-image: url(:/images/AMBIENT_ON_OFF.png);");

        // Power OFF -> color/brightness OFF
        handleAmbientColorButton(w, "");
        handleAmbientBrightnessButton(w, "");
    }
}

// 색상 버튼 처리
void handleAmbientColorButton(MainWindow* w, const QString& color)
{
    auto ui = w->getUi();
    
    ui->pushButton_ambient_red->setText("");
    ui->pushButton_ambient_yellow->setText("");
    ui->pushButton_ambient_green->setText("");
    ui->pushButton_ambient_rainbow->setText("");

    ui->pushButton_ambient_red->setStyleSheet("border-image: url(:/images/AMBIENT_RED_OFF.png);");
    ui->pushButton_ambient_yellow->setStyleSheet("border-image: url(:/images/AMBIENT_YELLOW_OFF.png);");
    ui->pushButton_ambient_green->setStyleSheet("border-image: url(:/images/AMBIENT_GREEN_OFF.png);");
    ui->pushButton_ambient_rainbow->setStyleSheet("border-image: url(:/images/AMBIENT_RAINBOW_OFF.png);");

    if      (color.compare("red",     Qt::CaseInsensitive)==0) ui->pushButton_ambient_red->setStyleSheet("border-image: url(:/images/AMBIENT_RED_ON.png);");
    else if (color.compare("yellow",  Qt::CaseInsensitive)==0) ui->pushButton_ambient_yellow->setStyleSheet("border-image: url(:/images/AMBIENT_YELLOW_ON.png);");
    else if (color.compare("green",   Qt::CaseInsensitive)==0) ui->pushButton_ambient_green->setStyleSheet("border-image: url(:/images/AMBIENT_GREEN_ON.png);");
    else if (color.compare("rainbow", Qt::CaseInsensitive)==0) ui->pushButton_ambient_rainbow->setStyleSheet("border-image: url(:/images/AMBIENT_RAINBOW_ON.png);");
}

void handleAmbientBrightnessButton(MainWindow* w, const QString& brightness)
{
    auto ui = w->getUi();

    ui->pushButton_ambient_low   ->setStyleSheet("border-image: url(:/images/ambient_low_off.png);");
    ui->pushButton_ambient_medium->setStyleSheet("border-image: url(:/images/ambient_medium_off.png);");
    ui->pushButton_ambient_high  ->setStyleSheet("border-image: url(:/images/ambient_high_off.png);");

    if      (brightness.compare("LOW",    Qt::CaseInsensitive)==0) ui->pushButton_ambient_low   ->setStyleSheet("border-image: url(:/images/ambient_low_on.png);");
    else if (brightness.compare("MEDIUM", Qt::CaseInsensitive)==0) ui->pushButton_ambient_medium->setStyleSheet("border-image: url(:/images/ambient_medium_on.png);");
    else if (brightness.compare("HIGH",   Qt::CaseInsensitive)==0) ui->pushButton_ambient_high  ->setStyleSheet("border-image: url(:/images/ambient_high_on.png);");
}
