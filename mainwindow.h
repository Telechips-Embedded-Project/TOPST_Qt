#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QPushButton>
#include <QEvent>
#include <QTimer>

#include "system_status.h"
#include "ripplewidget.h"

#include "account/accountsettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    MainWindow(const AccountSettings &settings, QWidget *parent = nullptr);
    ~MainWindow();

    // 외부에서 UI 접근용
    Ui::MainWindow* getUi() const { return ui; }

    // 에어컨 ON/OFF 상태 getter/setter
    bool isAirconOn() const { return airconIsOn; }
    void setAirconOn(bool on) { airconIsOn = on; }

    // 엠비언트 ON/OFF 상태 getter/setter
    bool isAmbientOn() const { return ambientIsOn; }
    void setAmbientOn(bool on) { ambientIsOn = on; }

    void sendJsonToFifo(const QString &jsonString);

private slots:
    void updateDateTime(); // Clock

    void on_btn_aircon_clicked();
    void on_btn_ambient_clicked();
    void on_btn_window_clicked();
    void on_btn_headlamp_clicked();
    void on_btn_music_clicked();
    void on_btn_video_clicked();
    void on_btn_trunk_clicked();
    void on_btn_bluetooth_clicked();
    void on_btn_settings_clicked();
    void on_btn_Navi_clicked();
    void on_btn_Enter_clicked();

    void on_pushButton_Play_Pause_clicked();
    void on_pushButton_Stop_clicked();
    void on_pushButton_Volume_clicked();
    void on_horizontalSlider_Volume_valueChanged(int value);
    void on_pushButton_Seek_Backward_clicked();
    void on_pushButton_Seek_Forward_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *m_timer;

    RippleWidget *ripple;

    void highlightOnly(QPushButton* btn);

    // 에어컨 ON/OFF 상태 저장
    bool airconIsOn = false;

    // 엠비언트 ON/OFF 상태 저장
    bool ambientIsOn = false;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void slideToPage(int index);

    void mousePressEvent(QMouseEvent *event) override;
    
};

#endif // MAINWINDOW_H
