#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QPushButton>
#include <QEvent>
#include <QTimer>

#include "system_status.h"
#include "ripplewidget.h"
#include "update_handler.h"

#include "notificationbanner.h"

// Naviiii
#include "mapview.h"
#include <QListWidget>
#include <QSet>
#include <QPair>
#include <QMessageBox>
#include <QMediaPlayer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class GamePage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 외부에서 UI 접근용
    Ui::MainWindow* getUi() const { return ui; }

    // 에어컨 ON/OFF 상태 getter/setter
    bool isAirconOn() const { return airconIsOn; }
    void setAirconOn(bool on) { airconIsOn = on; }

    // 엠비언트 ON/OFF 상태 getter/setter
    bool isAmbientOn() const { return ambientIsOn; }
    void setAmbientOn(bool on) { ambientIsOn = on; }

    // FIFO
    void sendJsonToFifo(const QString &jsonString);

    // 계정 선택 시 호출
    void setCurrentUser(int accountId);

    // shm memory - user_state_t 세터 (Settings 페이지에서 호출)
    void setUserAirconAuto(int on);
    void setUserAirconTemp(int t);
    void setUserAmbientAuto(int on);
    void setUserAmbientColorName(const char* name);
    void setUserWindowAuto(int on);
    void setUserWiperAuto(int on);
    void setUserWallpaperFlag(int on);
    void setUserWallpaperNum(int idx);
    // (디버그용)
    void dumpUserState(const char* tag = nullptr);

    // MainWindow에 “현재 계정 id” getter + 진입 시 복원 호출
    int accountId() const { return m_accountId; }

    // Naviiii
    MapView* getMapView() const { return m_mapView; }
    void handleSpeedLimitUI(int curSpeedLimit, int currentSpeed);
    void handleTunnelAlert(bool isTunnelNow, const QString& wayName, qint64 prevNid, qint64 currNid, double wayLength);

    void startRouteSimulation();

    // Co2 Flag
    void showNotification(const QString& msg, const QColor& accent = QColor("#00aaff"), int msec=3500);


private slots:
    void updateDateTime(); // Clock

    void on_btn_aircon_clicked();
    void on_btn_ambient_clicked();
    void on_btn_window_clicked();
    void on_btn_headlamp_clicked();
    void on_btn_music_clicked();
    void on_btn_video_clicked();
    void on_btn_trunk_clicked();
    void on_btn_game_clicked();
    void on_btn_settings_clicked();
    void on_btn_Navi_clicked();
    void on_btn_home_clicked();

    void on_pushButton_Play_Pause_clicked();
    void on_pushButton_Stop_clicked();
    void on_pushButton_Volume_clicked();
    void on_horizontalSlider_Volume_valueChanged(int value);
    void on_pushButton_Seek_Backward_clicked();
    void on_pushButton_Seek_Forward_clicked();

    void on_pushButton_user_clicked();


private:
    Ui::MainWindow *ui;
    QTimer *m_timer;

    RippleWidget *ripple;

    void highlightOnly(QPushButton* btn);

    // 에어컨 ON/OFF 상태 저장
    bool airconIsOn = false;

    // 엠비언트 ON/OFF 상태 저장
    bool ambientIsOn = false;

    NotificationBanner *m_banner;

    // account
    int     m_accountId = -1;
    QString m_accountName;
    QString m_avatarPath;

    void applyUserButtonAvatar(const QString& path);
    static QPixmap makeRounded(const QString& path, const QSize& size, int radius);
    

    // Naviiii
    MapView* m_mapView = nullptr;
    QVector<Node> m_nodes;
    QVector<Way> m_ways;

    QTimer* m_simulateTimer = nullptr;
    int m_currentRouteIdx = 0;
    QListWidget* m_recentPlacesList = nullptr;  // 최근 방문 장소 목록 (미정)
    void loadMapData();
    void routeTest();
    QSet<QPair<qint64, qint64>> m_tunnelEdges;

    QVector<Node> parseNodesFromCsv(const QString& filename);
    QVector<Way> parseWaysFromCsv(const QString& filename);
    QSet<QPair<qint64, qint64>> extractTunnelEdges(const QVector<Way>& ways);

    int m_prevSpeedLimit = -1;                  // 현재 적용 제한속도
    bool m_inTunnel = false;                     // 터널 구간 진입 상태

    QMessageBox* m_tunnelPopup = nullptr;        // 터널 진입 팝업
    QMessageBox* m_speedLimitPopup = nullptr;    // 제한속도 안내 팝업

    int getCurrentSpeedLimit(qint64 nodeId);     // 제한속도 반환 함수 (선언)
    QLabel* speedLimitLabel = nullptr;

    QMediaPlayer* m_speedAlertPlayer = nullptr;
    bool m_speedAlertOn = false;    
    void playSpeedAlert();
    void stopSpeedAlert();

    /* Game */
    GamePage* m_gamePage = nullptr;

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void slideToPage(int index);

    void mousePressEvent(QMouseEvent *event) override;
    
};

#endif // MAINWINDOW_H
