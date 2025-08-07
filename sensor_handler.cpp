#include "sensor_handler.h"
#include "system_status.h"       // SHM_KEY, system_status_t
#include "mainwindow.h"
#include "ui_mainwindow.h"       // label_sensor 등 멤버 사용 가능

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <QTimer>

system_status_t *shm_ptr = nullptr;
static QTimer *sensorTimer = nullptr;

void initSharedMemory()
{
    int shmid = shmget(SHM_KEY, sizeof(system_status_t), 0666);
    if (shmid == -1)
    {
        qDebug() << "[ERROR] shmget failed";
        shm_ptr = nullptr;
    }
    else
    {
        shm_ptr = (system_status_t *)shmat(shmid, nullptr, 0);
        if (shm_ptr == (void *)-1)
        {
            qDebug() << "[ERROR] shmat failed";
            shm_ptr = nullptr;
        }
        else
        {
            qDebug() << "[INFO] Shared memory attached successfully";
        }
    }
}

void startSensorTimer(MainWindow *w)
{
    if (!w) return;

    sensorTimer = new QTimer(w);
    QObject::connect(sensorTimer, &QTimer::timeout, [=]() {
        updateSensorLabel(w);
    });
    sensorTimer->start(1000);
}

void updateSensorLabel(MainWindow* w)
{
    if (!shm_ptr || !w) return;

    Ui::MainWindow* ui = w->getUi();
    if (!ui) return;

    const sensor_state_t &sensor = shm_ptr->sensor;

    QString text = QString("🌡 %1°C    💧 %2%")
        .arg(sensor.temperature, 0, 'f', 1)
        .arg(sensor.humidity, 0, 'f', 1);

    ui->label_sensor->setText(text);
}
