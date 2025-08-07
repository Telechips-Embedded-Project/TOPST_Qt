#ifndef WINDOW_PAGE_H
#define WINDOW_PAGE_H

#include <QPushButton>

class MainWindow;

void initWindowConnections(MainWindow* w);

void onWindowOpenPressed(MainWindow* w);
void onWindowOpenReleased(MainWindow* w);
void onWindowClosePressed(MainWindow* w);
void onWindowCloseReleased(MainWindow* w);

#endif // WINDOW_PAGE_H
