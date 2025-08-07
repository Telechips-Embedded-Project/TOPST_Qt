#ifndef TRUNK_PAGE_H
#define TRUNK_PAGE_H

#include <QPushButton>

class MainWindow;

void initTrunkConnections(MainWindow* w);
void handleTrunkPowerButton(MainWindow* w, QPushButton* sender);

#endif // TRUNK_PAGE_H
