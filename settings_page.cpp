#include "maincontainer.h"
#include "settings_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "db/db.h"

#include <QDebug>

#include <QtGlobal>
#include <QObject>
#include <QVector>
#include <QPushButton>
#include <QLabel>
#include <QLCDNumber>

// ====== Constants & Helpers ======

static const char* kAmbientNames[]   = { "red", "yellow", "green", "rainbow" };
static const char* kWallpaperNames[] = { "darkwave","galaxy","wood","thunder","mountain","city" };

static int clampTemp(int t) { return qBound(16, t, 30); }

static int ambientIndexFromName(const QString& name)
{
    const QString s = name.toLower();
    for (int i=0;i<4;++i) {
        if (s == kAmbientNames[i]) return i;
    }
    // fallback (unknown → red)
    return 0;
}

// ====== Public API (declared in settings_page.h) ======

void settingsRestoreFromDb(MainWindow* w)
{
    if (!w) return;
    Db::initUserDb(w->accountId());

    Db::SettingsRow row;
    if (!Db::loadSettings(w->accountId(), &row)) return;

    auto ui = w->getUi(); if (!ui) return;

    // 신호 차단(토글 setChecked가 toggled 신호를 내보내지 않게)
    QSignalBlocker b1(ui->pushButton_aircon_smartmode);
    QSignalBlocker b2(ui->pushButton_ambient_smartmode);
    QSignalBlocker b3(ui->pushButton_window_smartmode);
    QSignalBlocker b4(ui->pushButton_wiper_smartmode);
    QSignalBlocker b5(ui->pushButton_wallpaper_smartmode);

    if (ui->lcdNumber_temperature)
        ui->lcdNumber_temperature->display(row.aircon);
    if (ui->label_settings_color)
        ui->label_settings_color->setText(QString::fromUtf8(kAmbientNames[row.ambient]));
    if (ui->label_settings_wallpaper)
        ui->label_settings_wallpaper->setText(QString::fromUtf8(kWallpaperNames[row.wallpaper]));

    if (ui->pushButton_aircon_smartmode)    ui->pushButton_aircon_smartmode->setChecked(row.aircon_flag);
    if (ui->pushButton_ambient_smartmode)   ui->pushButton_ambient_smartmode->setChecked(row.ambient_flag);
    if (ui->pushButton_window_smartmode)    ui->pushButton_window_smartmode->setChecked(row.window_flag);
    if (ui->pushButton_wiper_smartmode)     ui->pushButton_wiper_smartmode->setChecked(row.wiper_flag);
    if (ui->pushButton_wallpaper_smartmode) ui->pushButton_wallpaper_smartmode->setChecked(row.wallpaper_flag);
}

void settingsApplyToShm(MainWindow* w)
{
    if (!w) return;

    Db::SettingsRow row;
    if (!Db::loadSettings(w->accountId(), &row)) return;

    // Aircon
    w->setUserAirconAuto(row.aircon_flag ? 1 : 0);
    if (row.aircon_flag) {
        w->setUserAirconTemp(row.aircon);          // 예: 18/24/30 등
    }

    // Ambient
    w->setUserAmbientAuto(row.ambient_flag ? 1 : 0);
    if (row.ambient_flag) {
        w->setUserAmbientColorName(kAmbientNames[row.ambient]); // "red"/"yellow"/"green"/"rainbow"
    }

    // Window / Wiper
    w->setUserWindowAuto(row.window_flag ? 1 : 0); // OFF면 0으로 내려서 디바이스 쪽에서 끄게 됨
    w->setUserWiperAuto (row.wiper_flag  ? 1 : 0);

    // Wallpaper
    w->setUserWallpaperFlag(row.wallpaper_flag ? 1 : 0);
    if (row.wallpaper_flag) {
        w->setUserWallpaperNum(row.wallpaper);
    }

    // 디버그 덤프
    w->dumpUserState("apply_on_account_switch");
}

// 온도 표시/저장 + (Smart ON이면) user_state_t에 현재 온도 반영
void updateTemperatureDisplay(MainWindow* w, int delta)
{
    if (!w) return;

    Db::SettingsRow row;
    Db::loadSettings(w->accountId(), &row);

    row.aircon = clampTemp(row.aircon + delta);
    Db::saveSettings(w->accountId(), row);

    // UI
    if (auto ui = w->getUi(); ui && ui->lcdNumber_temperature)
        ui->lcdNumber_temperature->display(row.aircon);

    // Smart ON이면 SHM(user_state_t)에 현재 온도 반영
    if (row.aircon_flag) {
        w->setUserAirconTemp(row.aircon);
        w->dumpUserState("aircon_temp_change");
    }
}

// 색상 라벨/저장 + (Smart ON이면) user_state_t에 색 문자열 반영
void updateAmbientDisplay(MainWindow* w, const char* color)
{
    if (!w || !color) return;

    const int idx = ambientIndexFromName(QString::fromUtf8(color));

    Db::SettingsRow row;
    Db::loadSettings(w->accountId(), &row);
    row.ambient = idx;
    Db::saveSettings(w->accountId(), row);

    // UI
    if (auto ui = w->getUi(); ui && ui->label_settings_color)
        ui->label_settings_color->setText(QString::fromUtf8(kAmbientNames[idx]));

    // Smart ON이면 SHM(user_state_t)에 색 문자열 반영
    if (row.ambient_flag) {
        w->setUserAmbientColorName(kAmbientNames[idx]);  // user.ambient_color[16]
        w->dumpUserState("ambient_set_color");
    }
}

// 배경 라벨/저장 + (Smart ON이면) user_state_t.wallpaper_num 반영
void updateWallpaperDisplay(MainWindow* w, int index)
{
    if (!w) return;

    const int idx = qBound(0, index, 5);

    Db::SettingsRow row;
    Db::loadSettings(w->accountId(), &row);
    row.wallpaper = idx;
    Db::saveSettings(w->accountId(), row);

    // UI
    if (auto ui = w->getUi(); ui && ui->label_settings_wallpaper)
        ui->label_settings_wallpaper->setText(QString::fromUtf8(kWallpaperNames[idx]));

    // Smart ON이면 SHM(user_state_t)에 번호 반영
    if (row.wallpaper_flag) {
        w->setUserWallpaperNum(idx);
        w->dumpUserState("wallpaper_set");
    }
}

// ====== Wire up all UI events ======

void initSettingsConnections(MainWindow* w)
{
    if (!w) return;
    auto ui = w->getUi();
    if (!ui) return;

    // --- Aircon buttons ---
    QObject::connect(ui->pushButton_tempup, &QPushButton::clicked, [=](){
        updateTemperatureDisplay(w, +1);
    });
    QObject::connect(ui->pushButton_tempdown, &QPushButton::clicked, [=](){
        updateTemperatureDisplay(w, -1);
    });

    // --- Aircon smart toggle ---
    QObject::connect(ui->pushButton_aircon_smartmode, &QPushButton::toggled, [=](bool on){
        Db::SettingsRow row; Db::loadSettings(w->accountId(), &row);
        row.aircon_flag = on ? 1 : 0;
        Db::saveSettings(w->accountId(), row);

        // user_state_t: auto 플래그 갱신, ON이면 현재 온도 스냅샷 반영
        w->setUserAirconAuto(on ? 1 : 0);
        if (on) w->setUserAirconTemp(row.aircon);
        w->dumpUserState("aircon_smart_toggle");
    });

    // --- Ambient color buttons ---
    QObject::connect(ui->pushButton_settings_red,     &QPushButton::clicked, [=](){ updateAmbientDisplay(w, "red"); });
    QObject::connect(ui->pushButton_settings_yellow,  &QPushButton::clicked, [=](){ updateAmbientDisplay(w, "yellow"); });
    QObject::connect(ui->pushButton_settings_green,   &QPushButton::clicked, [=](){ updateAmbientDisplay(w, "green"); });
    QObject::connect(ui->pushButton_settings_rainbow, &QPushButton::clicked, [=](){ updateAmbientDisplay(w, "rainbow"); });

    // --- Ambient smart toggle ---
    QObject::connect(ui->pushButton_ambient_smartmode, &QPushButton::toggled, [=](bool on){
        Db::SettingsRow row; Db::loadSettings(w->accountId(), &row);
        row.ambient_flag = on ? 1 : 0;
        Db::saveSettings(w->accountId(), row);

        // user_state_t: auto 플래그 갱신, ON이면 현재 색상 스냅샷 반영
        w->setUserAmbientAuto(on ? 1 : 0);
        if (on) w->setUserAmbientColorName(kAmbientNames[row.ambient]);
        w->dumpUserState("ambient_smart_toggle");
    });

    // --- Window smart toggle ---
    QObject::connect(ui->pushButton_window_smartmode, &QPushButton::toggled, [=](bool on){
        Db::SettingsRow row; Db::loadSettings(w->accountId(), &row);
        row.window_flag = on ? 1 : 0;
        Db::saveSettings(w->accountId(), row);

        // user_state_t: window_autoflag
        w->setUserWindowAuto(on ? 1 : 0);
        w->dumpUserState("window_smart_toggle");
    });

    // --- Wiper smart toggle ---
    QObject::connect(ui->pushButton_wiper_smartmode, &QPushButton::toggled, [=](bool on){
        Db::SettingsRow row; Db::loadSettings(w->accountId(), &row);
        row.wiper_flag = on ? 1 : 0;
        Db::saveSettings(w->accountId(), row);

        // user_state_t: wiper_autoflag
        w->setUserWiperAuto(on ? 1 : 0);
        w->dumpUserState("wiper_smart_toggle");
    });

    // --- Wallpaper buttons ---
    struct WPItem { QPushButton* b; int idx; };
    QVector<WPItem> wp = {
        { ui->pushButton_bg_darkwave, 0 },
        { ui->pushButton_bg_galaxy,   1 },
        { ui->pushButton_bg_wood,     2 },
        { ui->pushButton_bg_thunder,  3 },
        { ui->pushButton_bg_mountain, 4 },
        { ui->pushButton_bg_city,     5 },
    };
    for (const auto& it : wp) {
        QObject::connect(it.b, &QPushButton::clicked, [=](){
            updateWallpaperDisplay(w, it.idx);
            // 배경 이미지를 실제로 바꾸는 로직이 있다면 여기서 호출
            // (예: MainContainer 등에 setBackground 적용)
        });
    }

    // --- Wallpaper smart toggle ---
    QObject::connect(ui->pushButton_wallpaper_smartmode, &QPushButton::toggled, [=](bool on){
        Db::SettingsRow row; Db::loadSettings(w->accountId(), &row);
        row.wallpaper_flag = on ? 1 : 0;
        Db::saveSettings(w->accountId(), row);

        // user_state_t: wallpaper_flag + (ON이면) 현재 번호 스냅샷
        w->setUserWallpaperFlag(on ? 1 : 0);
        if (on) w->setUserWallpaperNum(row.wallpaper);
        w->dumpUserState("wallpaper_smart_toggle");
    });
}