#ifndef AIRCON_PAGE_H
#define AIRCON_PAGE_H

#include <QString>
#include <QPushButton>

class MainWindow;

void initAirconConnections(MainWindow* w);
void handleAirconPowerButton(MainWindow* w, QPushButton* sender);
void handleAirconLevelButton(MainWindow* w, const QString& level);
void setAirconGif(MainWindow* w, const QString& path, bool isGif);

#endif
