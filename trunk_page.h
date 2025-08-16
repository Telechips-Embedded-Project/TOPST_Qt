#ifndef TRUNK_PAGE_H
#define TRUNK_PAGE_H

#include <QString>

class MainWindow;

void initTrunkConnections(MainWindow* w);

void trunkStart(MainWindow* w,
                const QString& device = "/dev/video1",
                int width = 1280, int height = 720, int fps = 30);
void trunkStop(MainWindow* w);

#endif // TRUNK_PAGE_H
