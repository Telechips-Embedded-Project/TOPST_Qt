#ifndef AMBIENT_PAGE_H
#define AMBIENT_PAGE_H

#include <QString>
#include <QPushButton>

class MainWindow;

void initAmbientConnections(MainWindow* w);
void handleAmbientPowerButton(MainWindow* w, QPushButton* sender);
void handleAmbientColorButton(MainWindow* w, const QString& color);

#endif // AMBIENT_PAGE_H
