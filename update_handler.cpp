#include "update_handler.h"
#include "system_status.h"       // SHM_KEY, system_status_t
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aircon_page.h"
#include "ambient_page.h"
#include "headlamp_page.h"

#include "ambient_preview.h"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <QTimer>
#include <QDebug>
#include <QtGlobal>

system_status_t *shm_ptr = nullptr;
static QTimer *sensorTimer = nullptr;

extern void ambientSetLastColor(const QString& c);

void initSharedMemory()
{
    int shmid = shmget(SHM_KEY, sizeof(system_status_t), IPC_CREAT | 0666);
    if (shmid == -1) {
        qDebug() << "[ERROR] shmget failed";
        shm_ptr = nullptr;
        return;
    }

    shm_ptr = (system_status_t *)shmat(shmid, nullptr, 0);
    if (shm_ptr == (void *)-1) {
        qDebug() << "[ERROR] shmat failed";
        shm_ptr = nullptr;
        return;
    }

    qDebug() << "[INFO] Shared memory attached successfully"
             << "sizeof(system_status_t)=" << sizeof(system_status_t);
}

void startSensorTimer(MainWindow *w)
{
    if (!w) return;

    sensorTimer = new QTimer(w);
    QObject::connect(sensorTimer, &QTimer::timeout, [=]() {
        updateSensorLabel(w);
        updateButtonimage(w);
    });
    sensorTimer->start(500);
}

void updateSensorLabel(MainWindow* w)
{
    if (!shm_ptr || !w) return;
    Ui::MainWindow* ui = w->getUi(); if (!ui) return;

    const int t = int(shm_ptr->sensor.temperature);
    const int h = int(shm_ptr->sensor.humidity);

    // ---- UI 출력 ----
    const int FONT_PX = 26;
    ui->label_sensor->setTextFormat(Qt::RichText);
    ui->label_sensor->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->label_sensor->setStyleSheet(
        QString("color:white; padding-bottom:2px; font-size:%1px;").arg(FONT_PX));

    QString html = QString(
        "<span style='white-space:nowrap; vertical-align:middle;'>"
        "<span style=\"font-family:'Noto Emoji'; position:relative; top:-1px;\">🌡</span>&nbsp;%1°C"
        "&nbsp;&nbsp;<span style=\"font-family:'Noto Emoji'; position:relative; top:-1px;\">💧</span>&nbsp;%2%"
        "</span>").arg(t).arg(h);

    ui->label_sensor->setText(html);

    QFontMetrics fm(ui->label_sensor->font());
    int need = fm.size(Qt::TextSingleLine, QStringLiteral("100°C    100%")).width();
    ui->label_sensor->setMinimumWidth(need + 40);
    ui->label_sensor->setMinimumHeight(FONT_PX + 12);
}


void updateButtonimage(MainWindow* w)
{
    if (!shm_ptr || !w) return;
    Ui::MainWindow* ui = w->getUi();
    if (!ui) return;

    // ----- Aircon -----
    const int acLevel = shm_ptr->aircon.level;   // 0=OFF, 1=LOW, 2=MEDIUM, 3=HIGH (4=FAST 있으면 분기 추가)
    static int lastAcLevel = -1;

    if (acLevel != lastAcLevel) {
        if (acLevel <= 0) {
            handleAirconPowerButton(w, ui->pushButton_aircon_off);
            handleAirconLevelButton(w, "");

            ui->pushButton_aircon_low->setEnabled(false);
            ui->pushButton_aircon_medium->setEnabled(false);
            ui->pushButton_aircon_high->setEnabled(false);
        } else {
            handleAirconPowerButton(w, ui->pushButton_aircon_on);

            ui->pushButton_aircon_low->setEnabled(true);
            ui->pushButton_aircon_medium->setEnabled(true);
            ui->pushButton_aircon_high->setEnabled(true);

            if      (acLevel == 1) handleAirconLevelButton(w, "LOW");
            else if (acLevel == 2) handleAirconLevelButton(w, "MEDIUM");
            else                   handleAirconLevelButton(w, "HIGH");
        }
        lastAcLevel = acLevel;
    }


    // ----- Ambient -----
    const int amblevel = shm_ptr->ambient.brightness_level; // 0=OFF, 1=LOW, 2=MEDIUM, 3=HIGH
    const QString ambcolorRaw = QString::fromLatin1(shm_ptr->ambient.color[0] ? shm_ptr->ambient.color : "");
    const QString ambcolor = ambcolorRaw.trimmed().toLower();

    static int     lastAmbLevel = -1;
    static QString lastAmbColor;

    auto pv = w->findChild<AmbientPreview*>("previewAmbient");

    // 색상이 비었거나 "off"면 OFF로 판단
    const bool ambOn = (!ambcolor.isEmpty() && ambcolor != "off");

    // 1) 색상 변화가 있으면 최우선 갱신
    if (ambcolor != lastAmbColor) {
        if (!ambOn) {
            // OFF
            handleAmbientPowerButton(w, ui->pushButton_ambient_off);
            if (pv) pv->applyState(0, "");
            lastAmbLevel = 0;
        } else {
            // ON
            handleAmbientPowerButton(w, ui->pushButton_ambient_on);
            handleAmbientColorButton(w, ambcolor.isEmpty() ? "red" : ambcolor);

            // brightness
            int lvl = qBound(0, amblevel, 3);
            if      (lvl == 1) handleAmbientBrightnessButton(w, "LOW");
            else if (lvl == 2) handleAmbientBrightnessButton(w, "MEDIUM");
            else if (lvl >= 3) handleAmbientBrightnessButton(w, "HIGH");

            if (pv) pv->applyState(lvl, ambcolor.isEmpty() ? "red" : ambcolor);
            lastAmbLevel = lvl;

            ambientSetLastColor(ambcolor);
        }
        lastAmbColor = ambcolor;
    }
    // 2) 색상은 그대로고, 밝기만 바뀐 경우
    else if (ambOn && amblevel != lastAmbLevel) {
        int lvl = qBound(0, amblevel, 3);
        if      (lvl == 1) handleAmbientBrightnessButton(w, "LOW");
        else if (lvl == 2) handleAmbientBrightnessButton(w, "MEDIUM");
        else if (lvl >= 3) handleAmbientBrightnessButton(w, "HIGH");

        if (pv) pv->applyState(lvl, ambcolor.isEmpty() ? "red" : ambcolor);
        lastAmbLevel = lvl;
    }

    // ----- Headlamp -----
    const int Hlblevel = shm_ptr->headlamp.level; // 0=OFF, 1=ON

    static int     lastHlLevel = -1;

    if (Hlblevel != lastHlLevel)
    {
        if (Hlblevel <= 0) {
            handleHeadlampPowerButton(w, ui->pushButton_headlamp_off);
        } else {
            handleHeadlampPowerButton(w, ui->pushButton_headlamp_on);
        }

        lastHlLevel = Hlblevel;
    }
}