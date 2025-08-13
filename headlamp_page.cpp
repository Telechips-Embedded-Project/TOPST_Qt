#include "trunk_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "update_handler.h"
#include "system_status.h"

#include <QDebug>

void initHeadlampConnections(MainWindow* w)
{
    QObject::connect(w->getUi()->pushButton_headlamp_on, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"headlamp","command":"on","value":"N/A"})");

        /*
        if (!shm_ptr) return;
        shm_ptr->headlamp.level = 1;   // ON
        */
    });

    QObject::connect(w->getUi()->pushButton_headlamp_off, &QPushButton::clicked, [=]() {
        w->sendJsonToFifo(R"({"device":"headlamp","command":"off","value":"N/A"})");

        /*
        if (!shm_ptr) return;
        shm_ptr->headlamp.level = 0;   // OFF
        */
    });
}

void handleHeadlampPowerButton(MainWindow* w, QPushButton* sender)
{
    auto ui = w->getUi();

    ui->pushButton_headlamp_on->setText("");
    ui->pushButton_headlamp_off->setText("");

    if (sender == ui->pushButton_headlamp_on)
    {
        // ON Button Style
        ui->pushButton_headlamp_on->setStyleSheet("border-image: url(:/images/ON_ON.png);");
        ui->pushButton_headlamp_off->setStyleSheet("border-image: url(:/images/OFF_OFF.png);");
        ui->label_headlamp_preview->setStyleSheet("border-image: url(:/images/headlamp_preview_on.png);");
    }
    else
    {
        // OFF Button Style
        ui->pushButton_headlamp_on->setStyleSheet("border-image: url(:/images/ON_OFF.png);");
        ui->pushButton_headlamp_off->setStyleSheet("border-image: url(:/images/OFF_ON.png);");
        ui->label_headlamp_preview->setStyleSheet("border-image: url(:/images/headlamp_preview_off.png);");
    }
}
