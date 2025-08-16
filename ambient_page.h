#ifndef AMBIENT_PAGE_H
#define AMBIENT_PAGE_H

#include <QString>
#include <QPushButton>

class MainWindow;

void initAmbientConnections(MainWindow* w);
void handleAmbientPowerButton(MainWindow* w, QPushButton* sender);
void handleAmbientColorButton(MainWindow* w, const QString& color);
void handleAmbientBrightnessButton(MainWindow* w, const QString& brightness);

QString ambientGetLastColor();
void    ambientSetLastColor(const QString& c);

#endif // AMBIENT_PAGE_H
