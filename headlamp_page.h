#ifndef HEADLAMP_PAGE_H
#define HEADLAMP_PAGE_H

#include <QPushButton>

class MainWindow;

void initHeadlampConnections(MainWindow* w);
void handleHeadlampPowerButton(MainWindow* w, QPushButton* sender);

#endif // HEADLAMP_PAGE_H
