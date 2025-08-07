#include "trunk_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QDebug>
#include <QProcessEnvironment>

static QProcess* trunkCameraProcess = nullptr;

void initTrunkConnections(MainWindow* w)
{
    QObject::connect(w->getUi()->pushButton_trunk_camera_on, &QPushButton::clicked, [=]() {
        handleTrunkPowerButton(w, w->getUi()->pushButton_trunk_camera_on);
    });

    QObject::connect(w->getUi()->pushButton_trunk_camera_off, &QPushButton::clicked, [=]() {
        handleTrunkPowerButton(w, w->getUi()->pushButton_trunk_camera_off);
    });
}

void handleTrunkPowerButton(MainWindow* w, QPushButton* sender)
{
    auto ui = w->getUi();

    ui->pushButton_trunk_camera_on->setText("");
    ui->pushButton_trunk_camera_off->setText("");

    if (sender == ui->pushButton_trunk_camera_on)
    {
        if (trunkCameraProcess && trunkCameraProcess->state() == QProcess::Running)
            return;

        ui->pushButton_trunk_camera_on->setStyleSheet("border-image: url(:/images/ON_ON.png);");
        ui->pushButton_trunk_camera_off->setStyleSheet("border-image: url(:/images/OFF_OFF.png);");

        trunkCameraProcess = new QProcess(w);

        // DISPLAY 환경 변수 추가
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("DISPLAY", ":0");
        trunkCameraProcess->setProcessEnvironment(env);

        // 실행 명령어 및 인자 설정
        QString program = "libcamera-hello";
        QStringList args;
        args << "-t" << "0"
             << "--width" << "640"
             << "--height" << "480"
             << "--framerate" << "60"
             << "--info-text" << "%fps";

        trunkCameraProcess->start(program, args);

        if (!trunkCameraProcess->waitForStarted(1000)) {
            qDebug() << "[ERROR] Trunk camera failed to start";
            delete trunkCameraProcess;
            trunkCameraProcess = nullptr;
        } else {
            qDebug() << "[OK] Trunk camera started";
        }
    }
    else
    {
        // OFF 버튼일 경우
        // 실행 중인 프로세스 종료
        if (trunkCameraProcess && trunkCameraProcess->state() == QProcess::Running) {
            trunkCameraProcess->terminate();
            trunkCameraProcess->waitForFinished(1000);
            delete trunkCameraProcess;
            trunkCameraProcess = nullptr;

            qDebug() << "[OK] Trunk camera stopped";
        }

        ui->pushButton_trunk_camera_on->setStyleSheet("border-image: url(:/images/ON_OFF.png);");
        ui->pushButton_trunk_camera_off->setStyleSheet("border-image: url(:/images/OFF_ON.png);");
    }
}
