#ifndef UPDATE_HANDLER_H
#define UPDATE_HANDLER_H

#include <QString>

#include "mainwindow.h"

extern system_status_t *shm_ptr;

class MainWindow;

void initSharedMemory();
void startSensorTimer(MainWindow *w);
void updateSensorLabel(MainWindow* w);

// 추가 구현중
void updateButtonimage(MainWindow* w);

// Co2 Notification
void checkCo2Banner(MainWindow* w);

#endif // UPDATE_HANDLER_H
