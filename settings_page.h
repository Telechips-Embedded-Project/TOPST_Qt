#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <QWidget>

#include <QString>
#include <QPushButton>

class MainWindow;

void initSettingsConnections(MainWindow* w);
void updateTemperatureDisplay(MainWindow* w, int delta);
void updateAmbientDisplay(MainWindow* w, const char* color);
void updateWallpaperDisplay(MainWindow* w, int index);
void settingsRestoreFromDb(MainWindow* w);

void settingsApplyToShm(MainWindow* w);

#endif // SETTINGS_PAGE_H
