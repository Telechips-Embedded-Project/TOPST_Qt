#include "trunk_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

void initHeadlampConnections(MainWindow* w)
{
    QObject::connect(w->getUi()->pushButton_headlamp_on, &QPushButton::clicked, [=]() {
        handleTrunkPowerButton(w, w->getUi()->pushButton_headlamp_on);
    });

    QObject::connect(w->getUi()->pushButton_headlamp_off, &QPushButton::clicked, [=]() {
        handleTrunkPowerButton(w, w->getUi()->pushButton_headlamp_off);
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
        ui->pushButton_headlamp_on->setStyleSheet("border-image: url(:/images/OFF_OFF.png);");
    }
    else
    {
        // OFF Button Style
        ui->pushButton_headlamp_off->setStyleSheet("border-image: url(:/images/ON_OFF.png);");
        ui->pushButton_headlamp_off->setStyleSheet("border-image: url(:/images/OFF_ON.png);");
    }
}
