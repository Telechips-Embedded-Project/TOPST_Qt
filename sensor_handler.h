#ifndef SENSOR_HANDLER_H
#define SENSOR_HANDLER_H

#include <QString>

#include "mainwindow.h"

extern system_status_t *shm_ptr;

class MainWindow;

void initSharedMemory();
void startSensorTimer(MainWindow *w);
void updateSensorLabel(MainWindow* w);

#endif // SENSOR_HANDLER_H
