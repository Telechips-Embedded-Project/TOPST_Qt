#include "update_handler.h"
#include "system_status.h"       // SHM_KEY, system_status_t
#include "mainwindow.h"
#include "maincontainer.h"
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

#include <QLabel>
#include <QPixmap>
#include <QFile>
#include <QColor>

// -------------------------------------------------------
// ---------------------- Co2 alram ----------------------
// -------------------------------------------------------

static int        s_lastCo2Flag = -1;
static QDateTime  s_lastSent;

void checkCo2Banner(MainWindow* w)
{
    if (!w || !shm_ptr) return;

    const int f = int(shm_ptr->sensor.CO2_flag);   // 0: <1500, 1: ≥1500, 2: ≥2000
    if (f <= 0) {
        // 정상 구간(0)으로 내려온 경우에는 배너를 굳이 띄우지 않고 상태만 리셋
        s_lastCo2Flag = f;
        return;
    }

    // 같은 구간에서 500ms마다 반복 알림 방지: 값이 변했거나, 더 높은 단계로 상승했을 때만
    bool need = (f != s_lastCo2Flag) || (f > s_lastCo2Flag);

    // (선택) 같은 단계에서 최소 30초 쿨다운
    if (!need && s_lastSent.isValid() && s_lastSent.secsTo(QDateTime::currentDateTime()) < 30)
        return;

    QString msg;
    QColor  accent("#ffcc00"); // level 1: 경고(노랑)
    if (f == 1) {
        msg = QStringLiteral("CO₂ 1500ppm 이상입니다. 환기하세요.");
        accent = QColor("#ffcc00");
    } else { // f == 2
        msg = QStringLiteral("CO₂ 2000ppm 이상! 즉시 환기가 필요합니다.");
        accent = QColor("#ff5555");     // level 2: 위험(빨강)
    }

    w->showNotification(msg, accent, /*msec*/ 3500);
    s_lastCo2Flag = f;
    s_lastSent = QDateTime::currentDateTime();
}
// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------

// -------------------------------------------------------
// ------------------ Wallpaper helpers ------------------
// -------------------------------------------------------
static int g_lastWpFlag = -1;
static int g_lastWpNum  = -1;

static QString wallpaperPathFor(int num)
{
    switch (num) {
    case 0: return QString(":/images/bg_darkwave.png");
    case 1: return QString(":/images/bg_galaxy.png");
    case 2: return QString(":/images/bg_wood.png");
    case 3: return QString(":/images/bg_thunder.png");
    case 4: return QString(":/images/bg_mountain.png");
    case 5: return QString(":/images/bg_city.png");
    default: return QString();
    }
}

static void applyWallpaper(MainWindow* w, const QString& path)
{
    if (!w) return;

    // 1) MainContainer가 있으면 그것부터 시도 (이게 진짜 배경을 그림)
    if (auto mc = qobject_cast<MainContainer*>(w->centralWidget())) {
        if (path.isEmpty()) {
            mc->setBackground(QString());   // 기본/빈 배경(필요 시)
        } else {
            if (!QFile::exists(path)) {
                qWarning() << "[WALLPAPER] missing resource:" << path;
            }
            mc->setBackground(path);        // ★ 핵심: 컨테이너에 직접 지정
        }
        return;
    }

    // 2) 폴백: label_wallpaper 라벨이 있으면 라벨로
    if (QLabel* lb = w->findChild<QLabel*>("label_wallpaper")) {
        lb->setScaledContents(true);
        lb->setPixmap(QPixmap(path));
        lb->raise();
        lb->setVisible(true);
        return;
    }

    // 3) 최후의 폴백: centralwidget 스타일시트
    if (auto ui = w->getUi(); ui && ui->centralwidget) {
        ui->centralwidget->setStyleSheet(QString(
            "QWidget#centralwidget {"
            "  background-image: url(%1);"
            "  background-repeat: no-repeat;"
            "  background-position: center;"
            "  background-attachment: fixed;"
            "  background-color: black;"
            "}").arg(path));
    }
}

// -------------------------------------------------------
// -------------------------------------------------------
// -------------------------------------------------------


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
        checkCo2Banner(w);
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

    // ----- wallpaper -----
    const int wpFlag = shm_ptr->user.wallpaper_flag;   // 0:수동/미사용, 1:smart mode
    const int wpNum  = qBound(0, shm_ptr->user.wallpaper_num, 5);

    if (wpFlag != g_lastWpFlag || (wpFlag == 1 && wpNum != g_lastWpNum)) {
        // smart mode: on
        if (wpFlag == 1) {
            const QString path = wallpaperPathFor(wpNum);
            if (path.isEmpty()) {
                qDebug() << "[WALLPAPER] invalid number:" << wpNum;
                applyWallpaper(w, QString());
            } else {
                qDebug() << "[WALLPAPER] apply" << path;
                applyWallpaper(w, path);
            }
        }
        // smart mode: off
        else {
            qDebug() << "[WALLPAPER] smart mode OFF";
        }
        g_lastWpFlag = wpFlag;
        g_lastWpNum  = (wpFlag == 1 ? wpNum : g_lastWpNum);
    }
}
