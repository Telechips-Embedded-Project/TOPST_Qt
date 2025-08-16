#include "mainwindow.h"                         // class definition
#include "ui_mainwindow.h"                      // Access UI Member

#include "update_handler.h"                     // RealTime Update

#include "account/accountselectpage.h"          // select account
#include "db/db.h"                              // QSQLite db

#include "aircon_page.h"                        // Aircon
#include "ambient_page.h"                       // Ambient
#include "window_page.h"                        // window
#include "headlamp_page.h"                      // Headlamp
#include "trunk_page.h"                         // Trunk
#include "music_page.h"                         // Music
#include "video_page.h"                         // Video
#include "settings_page.h"                      // settings
#include "home_page.h"                          // home
#include "game_page.h"                          // game

#include <QPainterPath>                         // for makeRounded

#include <QGraphicsDropShadowEffect>            // Shadow Effect
#include <QTimer>                               // Timer
#include <QTime>                                // Clock
#include <QDate>                                // Date

#include <QUrl>
#include <QRegularExpression>
#include <QGroupBox>
#include <QImageReader>
#include <QFileInfoList>

#include <QResizeEvent>                         // resizeEvent

/* 3D OpenGL */
#include "car_obj_view.h"
#include <QOpenGLWidget>

/* banner effect */
#include <QShortcut>
#include <QKeyEvent>

/* slide effect */
#include <QPropertyAnimation>
#include <QEasingCurve>

/* mouse effect */
#include <QMouseEvent>
#include <QCursor>

/* FIFO header */
#include <QFile>
#include <QTextStream>

#include <unistd.h>                             // access(), F_OK
#include <sys/types.h>                          // mkfifo()에 필요
#include <sys/stat.h>                           // mkfifo()
#include <cstring>

// FIFO
#define PATH_FIFO "/tmp/qt_fifo"

// Naviii
#include "navi_page.h"                          // navigation
#include "navi_utils.h"   // calculateBearing 선언 포함

// 자연스럽게 움직이는지 테스트 0810
#include <cstdlib>     // rand(), srand()
#include <ctime>       // time()
#include <cmath>
#include <QtMath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_timer(new QTimer(this))
{
    qputenv("QT_GSTREAMER_USE_OVERLAY", "0");
    srand(static_cast<unsigned int>(time(nullptr))); // 0810

    ui->setupUi(this);

    //======================================================================================
    // =================================== car 3D OpenGL ===================================
    //======================================================================================
    auto lay = (ui->widget_car->layout()
                ? ui->widget_car->layout()
                : (QLayout*)new QVBoxLayout(ui->widget_car));
    lay->setContentsMargins(0,0,0,0);

    auto *car = new CarObjView(ui->widget_car);
    lay->addWidget(car);

    car->setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    car->setBackground(QColor(10,16,24));
    car->setModelColor(QColor(210,220,230));

    // 실행파일 디렉터리 기준으로 images/Cybertruck.obj 찾기
    const QString base = QCoreApplication::applicationDirPath();
    QString objPath = QDir(base).filePath("images/Cybertruck.obj");

    // 빌드/런 위치가 다르면 예비 경로도 시도
    if (!QFile::exists(objPath))
        objPath = QDir(base).filePath("../images/Cybertruck.obj");

    qDebug() << "[OBJ] try:" << objPath << "exists?" << QFile::exists(objPath);
    bool ok = car->loadObj(objPath);
    qDebug() << "[OBJ] load" << ok;
    if (ok) car->setInitialView(-30.f, 10.f, 1.75f);
    //======================================================================================
    // =====================================================================================
    //======================================================================================

    // === Game ===
    m_gamePage = new GamePage(
        ui->widget_game_select,
        ui->widget_game,
        ui->page_game
    );
    // ===========

    ripple = new RippleWidget(this);
    ripple->resize(this->size());
    ripple->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ripple->raise();

//    this->setGeometry(0, 0, 2560, 1600);                    // static size (x, y, width, height)
//    ui->centralwidget->setFixedSize(2560, 1600);            // static size (width, height)

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);         // 테두리 제거
//    this->show();
     this->showFullScreen();  // 전체 화면으로 시작
    // this->showMaximized(); // 화면 최대화 시작(테두리 있음)

    // 각 버튼에 이벤트 필터 등록 (마우스 진입/이탈 감지용)
    ui->btn_aircon->installEventFilter(this);
    ui->btn_ambient->installEventFilter(this);
    ui->btn_window->installEventFilter(this);
    ui->btn_headlamp->installEventFilter(this);
    ui->btn_music->installEventFilter(this);
    ui->btn_video->installEventFilter(this);
    ui->btn_trunk->installEventFilter(this);
    ui->btn_game->installEventFilter(this);
    ui->btn_settings->installEventFilter(this);
    ui->btn_Navi->installEventFilter(this);
    ui->btn_home->installEventFilter(this);

    // 공유 메모리 연결
    initSharedMemory();
    startSensorTimer(this);
    // updateButtonimage(this); // 필요시 주석 해제

    // init aircon_module
    initAirconConnections(this);

    // init ambient_module
    initAmbientConnections(this);

    // init window_module
    initWindowConnections(this);

    // init headlamp_module
    initHeadlampConnections(this);

    // init trunk_module
    initTrunkConnections(this);

    // init music_module
    initMusicConnections(this);

    // init video_module
    initVideoConnections(this);

    // init settings_module
    initSettingsConnections(this);

    // init home_module
    initHomePage(this);

    // init value
    updateDateTime();

    // update 1sec
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    m_timer->start(1000);

    // style
    ui->label_time->setStyleSheet("color: white; font-size: 50px; font-weight: bold;");
    ui->label_date->setStyleSheet("color: white; font-size: 38px; font-weight: bold;");
    ui->label_sensor->setStyleSheet("color: white; font-size: 28px; font-weight: bold;");

    // shadow effect (option)
    auto *shadowTitle = new QGraphicsDropShadowEffect(this);
    shadowTitle->setOffset(1, 1);
    shadowTitle->setBlurRadius(3);
    shadowTitle->setColor(Qt::black);

    // creat banner Widget
    m_banner = new NotificationBanner(this);
    // m_banner->setMessage("Test");
    // m_banner->setAccentColor(QColor("#ff5555"));

    // banner history (home_page)
    homeWireBanner(this, m_banner);

    /*
    // Test Trigger (Ctrl+Shift+N)
    auto sc = new QShortcut(QKeySequence("Ctrl+Shift+N"), this);
    connect(sc, &QShortcut::activated, this, [this]{
        shm_ptr->sensor.CO2_flag = 1;
        // m_banner->setMessage("Test: CO₂ 2500ppm 초과! 환기 시작");
        // m_banner->showBanner(3500);  // for 3.5 sec
    });

    auto sc2 = new QShortcut(QKeySequence("Ctrl+Shift+B"), this);
    connect(sc2, &QShortcut::activated, this, [this]{
        shm_ptr->sensor.CO2_flag = 2;
        // m_banner->setMessage("Test: CO₂ 2500ppm 초과! 환기 시작");
        // m_banner->showBanner(3500);  // for 3.5 sec
    });
    */

    // user btn style
    ui->pushButton_user->setFixedSize(64, 64);
    ui->pushButton_user->setStyleSheet(
        "QPushButton { border: none; border-radius: 32px; background: transparent; }"
        "QPushButton:hover { opacity: 0.9; }"
    );

    // init value FAN Image(not gif)
    setAirconGif(this, ":/gif/aircon_fan.png", true);


    // Naviiii
// ================ navigation init ================
    // 1) MapView 생성 및 배치
    m_mapView = new MapView(this);
    if (ui->mapContainer) {
        auto* layout = new QVBoxLayout();
        layout->setContentsMargins(0,0,0,0);
        layout->addWidget(m_mapView);
        ui->mapContainer->setLayout(layout);
    } else {
        setCentralWidget(m_mapView);
    }

    // 2) 지도 데이터 로드
    loadMapData();

    QListWidget* destList = ui->EnterWidget->widget(3)->findChild<QListWidget*>("list_dest_places");
    if (destList) {
        destList->clear(); 
    // 목적지 좌표 매핑
        QMap<QString, QPair<double,double>> placeCoords = {
            {"list1",  {37.5133, 127.1078}},
            {"list2",  {37.554722, 126.970833}},
            {"list3",  {37.497942, 127.027621}},
            {"gadi",   {37.4810, 126.8820}},
            {"pangyo", {37.3860, 127.1110}}
        };

        for (const QString& placeName : placeCoords.keys()) {
            QListWidgetItem* item = new QListWidgetItem(placeName);
            destList->addItem(item);
        }
    }

    // 3) 내비게이션 시그널-슬롯 초기화
    initNaviConnections(this);
    QMediaPlayer* m_speedAlertPlayer = nullptr;
    bool m_speedAlertOn = false;
}

// Date & Time
void MainWindow::updateDateTime()
{
    QTime time = QTime::currentTime();
    QString timeStr = time.toString("h:mm AP");  // Time

    QDate date = QDate::currentDate();
    QString dateStr = date.toString("M월 d일 (ddd)");  // Date

    ui->label_time->setText(timeStr);
    ui->label_date->setText(dateStr);
}

// Co2 Flag
void MainWindow::showNotification(const QString& msg, const QColor& accent, int msec)
{
    if (!m_banner) return;
    m_banner->setAccentColor(accent);
    m_banner->setMessage(msg);
    m_banner->showBanner(msec);
}

// Userbtn Icon style utils
QPixmap MainWindow::makeRounded(const QString& path, const QSize& size, int radius)
{
    QPixmap src(path);
    if (src.isNull()) { QPixmap tmp(size); tmp.fill(QColor(18,36,58)); src = tmp; }

    QPixmap scaled = src.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    QPixmap dst(size);
    dst.fill(Qt::transparent);

    QPainter p(&dst);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(QPointF(0,0), QSizeF(size)), radius, radius);
    p.setClipPath(clip);

    const qreal x = (scaled.width()  - size.width())  * 0.5;
    const qreal y = (scaled.height() - size.height()) * 0.5;
    p.drawPixmap(QPointF(-x, -y), scaled);
    p.end();
    return dst;
}

// Userbtn Icon style setter
void MainWindow::applyUserButtonAvatar(const QString& path)
{
    if (!ui || !ui->pushButton_user) return;
    const QSize sz = ui->pushButton_user->size();
    QPixmap rounded = makeRounded(path, sz, qMin(sz.width(), sz.height())/5); // 살짝 둥글게
    ui->pushButton_user->setIcon(QIcon(rounded));
    ui->pushButton_user->setIconSize(sz);
}

// Current User Settings
void MainWindow::setCurrentUser(int accountId)
{
    m_accountId = accountId;

    Db::Account acc;
    if (Db::getAccount(accountId, &acc)) {
        m_accountName = acc.name;
        m_avatarPath  = acc.avatar;
        if (m_avatarPath.isEmpty())
            m_avatarPath = ":/account/images/Robot.png";
        applyUserButtonAvatar(m_avatarPath);
    }

    // Settings UI/토글 복원
    settingsRestoreFromDb(this);

    // 디바이스(=SHM) 적용: Smart ON 항목만 실제 값 반영
    settingsApplyToShm(this);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    float scaleFactor_neon = 1.1;
    float scaleFactor = 1.3;

    // Aircon btn
    if (obj == ui->btn_aircon)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_aircon->setIcon(QIcon(":/images/Aircon_neon.png"));
            ui->btn_aircon->setIconSize(ui->btn_aircon->size() * scaleFactor_neon);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_aircon->setIcon(QIcon(":/images/Aircon.png"));
            ui->btn_aircon->setIconSize(ui->btn_aircon->size() * scaleFactor);
            return true;
        }
    }

    // Ambient btn
    else if (obj == ui->btn_ambient)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_ambient->setIcon(QIcon(":/images/Ambient_neon.png"));
            ui->btn_ambient->setIconSize(ui->btn_ambient->size() * scaleFactor_neon);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_ambient->setIcon(QIcon(":/images/Ambient.png"));
            ui->btn_ambient->setIconSize(ui->btn_ambient->size() * scaleFactor);
            return true;
        }
    }

    // Window btn
    else if (obj == ui->btn_window)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_window->setIcon(QIcon(":/images/Window_neon.png"));
            ui->btn_window->setIconSize(ui->btn_window->size() * scaleFactor_neon);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_window->setIcon(QIcon(":/images/Window.png"));
            ui->btn_window->setIconSize(ui->btn_window->size() * scaleFactor);
            return true;
        }
    }

    // Headlamp btn
    else if (obj == ui->btn_headlamp)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_headlamp->setIcon(QIcon(":/images/Headlamp_neon.png"));
            ui->btn_headlamp->setIconSize(ui->btn_headlamp->size() * scaleFactor_neon);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_headlamp->setIcon(QIcon(":/images/Headlamp.png"));
            ui->btn_headlamp->setIconSize(ui->btn_headlamp->size() * scaleFactor);
            return true;
        }
    }

    // Music btn
    else if (obj == ui->btn_music)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_music->setIcon(QIcon(":/images/Music_neon.png"));
            ui->btn_music->setIconSize(ui->btn_music->size() * 0.85);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_music->setIcon(QIcon(":/images/Music.png"));
            ui->btn_music->setIconSize(ui->btn_music->size() * 1);
            return true;
        }
    }

    // Video btn
    else if (obj == ui->btn_video)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_video->setIcon(QIcon(":/images/Video_neon.png"));
            ui->btn_video->setIconSize(ui->btn_video->size() * 0.85);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_video->setIcon(QIcon(":/images/Video.png"));
            ui->btn_video->setIconSize(ui->btn_video->size() * 1);
            return true;
        }
    }

    // Trunk btn
    else if (obj == ui->btn_trunk)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_trunk->setIcon(QIcon(":/images/Trunk_neon.png"));
            ui->btn_trunk->setIconSize(ui->btn_trunk->size() * 0.9);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_trunk->setIcon(QIcon(":/images/Trunk.png"));
            ui->btn_trunk->setIconSize(ui->btn_trunk->size() * 1);
            return true;
        }
    }

    // Game btn
    else if (obj == ui->btn_game)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_game->setIcon(QIcon(":/images/Game.png"));
            ui->btn_game->setIconSize(ui->btn_game->size() * scaleFactor_neon);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
//            ui->btn_game->setIcon(QIcon(":/images/Gmae.png"));
            ui->btn_game->setIconSize(ui->btn_game->size() * scaleFactor);
            return true;
        }
    }

    // Settings btn
    else if (obj == ui->btn_settings)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_settings->setIcon(QIcon(":/images/Settings_neon.png"));
            ui->btn_settings->setIconSize(ui->btn_settings->size() * 1.2);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_settings->setIcon(QIcon(":/images/Settings.png"));
            ui->btn_settings->setIconSize(ui->btn_settings->size() * scaleFactor);
            return true;
        }
    }

    // Navi btn
    else if (obj == ui->btn_Navi)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_Navi->setIcon(QIcon(":/images/Navi_neon.png"));
            ui->btn_Navi->setIconSize(ui->btn_Navi->size() * 0.7);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_Navi->setIcon(QIcon(":/images/Navi.png"));
            ui->btn_Navi->setIconSize(ui->btn_Navi->size() * 0.8);
            return true;
        }
    }

    // Home btn
    else if (obj == ui->btn_home)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_home->setIcon(QIcon(":/images/Home_neon.png"));
            ui->btn_home->setIconSize(ui->btn_home->size() * 0.9);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_home->setIcon(QIcon(":/images/Home.png"));
            ui->btn_home->setIconSize(ui->btn_home->size() * 0.9);
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QPoint globalPos = mouseEvent->globalPos();
        QPoint ripplePos = ripple->mapFromGlobal(globalPos);
        ripple->startRipple(ripplePos);
    }

    if (obj == getVideoWidget()) {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Show) {
            if (auto vw = getVideoWidget()) {
                if (auto rip = vw->findChild<RippleWidget*>()) {
                    rip->setGeometry(vw->rect());
                }
            }
        }
        return false; // 기본 처리 계속
    }

    // 처리 안 된 이벤트는 기본 처리
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (ripple)
        ripple->resize(this->size());

    // 버튼당 아이콘 크기 설정 비율
    float scaleFactor = 1.3;

    // Ambient
    ui->btn_ambient->setIcon(QIcon(":/images/Ambient.png"));
    ui->btn_ambient->setIconSize(ui->btn_ambient->size() * scaleFactor);
    ui->btn_ambient->setText("");

    // Aircon
    ui->btn_aircon->setIcon(QIcon(":/images/Aircon.png"));
    ui->btn_aircon->setIconSize(ui->btn_aircon->size() * scaleFactor);
    ui->btn_aircon->setText("");

    // Window
    ui->btn_window->setIcon(QIcon(":/images/Window.png"));
    ui->btn_window->setIconSize(ui->btn_window->size() * scaleFactor);
    ui->btn_window->setText("");

    // Headlamp
    ui->btn_headlamp->setIcon(QIcon(":/images/Headlamp.png"));
    ui->btn_headlamp->setIconSize(ui->btn_headlamp->size() * scaleFactor);
    ui->btn_headlamp->setText("");

    // Music
    ui->btn_music->setIcon(QIcon(":/images/Music.png"));
    ui->btn_music->setIconSize(ui->btn_music->size() * 1);
    ui->btn_music->setText("");

    // Video
    ui->btn_video->setIcon(QIcon(":/images/Video.png"));
    ui->btn_video->setIconSize(ui->btn_video->size() * 1);
    ui->btn_video->setText("");

    // Trunk
    ui->btn_trunk->setIcon(QIcon(":/images/Trunk.png"));
    ui->btn_trunk->setIconSize(ui->btn_trunk->size() * 1);
    ui->btn_trunk->setText("");

    // Game
    ui->btn_game->setIcon(QIcon(":/images/Game.png"));
    ui->btn_game->setIconSize(ui->btn_game->size() * scaleFactor);
    ui->btn_game->setText("");

    // Settings
    ui->btn_settings->setIcon(QIcon(":/images/Settings.png"));
    ui->btn_settings->setIconSize(ui->btn_settings->size() * scaleFactor);
    ui->btn_settings->setText("");

    // Navi
    ui->btn_Navi->setIcon(QIcon(":/images/Navi.png"));
    ui->btn_Navi->setIconSize(ui->btn_Navi->size() * 0.8);
    ui->btn_Navi->setText("");

    // Home
    ui->btn_home->setIcon(QIcon(":/images/Home.png"));
    ui->btn_home->setIconSize(ui->btn_home->size() * 0.9);
    ui->btn_home->setText("");
}

// == fifo transmit ==
void MainWindow::sendJsonToFifo(const QString &jsonString)
{
    // FIFO 파일이 없으면 생성
    if (access(PATH_FIFO, F_OK) != 0)
    {
        if (mkfifo(PATH_FIFO, 0666) < 0)
        {
            perror("mkfifo");
            return;
        }
    }

    // FIFO 열기
    QFile fifo(PATH_FIFO);
    if (!fifo.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        qDebug() << "FIFO open failed";
        return;
    }

    // JSON 문자열 쓰기
    QTextStream out(&fifo);
    out << jsonString + "\n";
    fifo.close();

    qDebug() << "Sent JSON to FIFO:" << jsonString;
}

// 버튼 클릭 슬롯들
void MainWindow::on_btn_aircon_clicked()
{
    qDebug() << "[INFO] Aircon button clicked!";
    highlightOnly(ui->btn_aircon);
    slideToPage(1);                       // page_aircon
}

void MainWindow::on_btn_ambient_clicked()
{
    qDebug() << "[INFO] Ambient button clicked!";
    highlightOnly(ui->btn_ambient);
    slideToPage(2);                       // page_ambient
}

void MainWindow::on_btn_window_clicked()
{
    qDebug() << "[INFO] Window button clicked!";
    highlightOnly(ui->btn_window);
    slideToPage(3);                       // page_window
}

void MainWindow::on_btn_headlamp_clicked()
{
    qDebug() << "[INFO] Head Lamp button clicked!";
    highlightOnly(ui->btn_headlamp);
    slideToPage(4);                       // page_headlamp
}

void MainWindow::on_btn_music_clicked()
{
    qDebug() << "[INFO] Music button clicked!";
    highlightOnly(ui->btn_music);
    ui->EnterWidget->setCurrentIndex(0);  // page_music
}

void MainWindow::on_btn_video_clicked()
{
    qDebug() << "[INFO] Video button clicked!";
    highlightOnly(ui->btn_video);
    ui->EnterWidget->setCurrentIndex(1);  // page_video
}

void MainWindow::on_btn_trunk_clicked()
{
    qDebug() << "[INFO] Trunk button clicked!";
    highlightOnly(ui->btn_trunk);
    ui->EnterWidget->setCurrentIndex(2);  // page_trunk
}

void MainWindow::on_btn_game_clicked()
{
    qDebug() << "[INFO] Game button clicked!";
    highlightOnly(ui->btn_game);
    slideToPage(5);                       // page_trunk
}

void MainWindow::on_btn_settings_clicked()
{
    qDebug() << "[INFO] Settings button clicked!";
    highlightOnly(ui->btn_settings);
    slideToPage(6);                       // settings_trunk
}

void MainWindow::on_btn_Navi_clicked()
{
    qDebug() << "[INFO] Navi button clicked!";
    highlightOnly(ui->btn_Navi);
    ui->EnterWidget->setCurrentIndex(3);  // page_navi
}

void MainWindow::on_btn_home_clicked()
{
    qDebug() << "[INFO] Enter button clicked!";
    highlightOnly(ui->btn_home);
    ui->EnterWidget->setCurrentIndex(4);  // page_enter
}

// 페이지 클릭 버튼 style 처리
void MainWindow::highlightOnly(QPushButton* btn)
{
    // control group
    QList<QPushButton*> controlButtons = {
        ui->btn_aircon,
        ui->btn_ambient,
        ui->btn_window,
        ui->btn_headlamp,
        ui->btn_game,
        ui->btn_settings,
    };

    // media group
    QList<QPushButton*> mediaButtons = {
        ui->btn_music,
        ui->btn_video,
        ui->btn_trunk,
        ui->btn_Navi,
        ui->btn_home
    };

    // 기본 스타일 (hover, pressed 포함)
    QString baseStyle = R"(
        QPushButton {
            background-color: rgba(58, 63, 70, 180);
            color: white;
            border: 1px solid #6d7a8a;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
            border-bottom-left-radius: 0px;
            border-bottom-right-radius: 0px;
            padding: 10px;
            font: bold 16px;
        }

        QPushButton:hover {
            background-color: #4a5059;
        }

        QPushButton:pressed {
            background-color: #2a2e33;
            border-top: 1px solid #6d7a8a;
            border-left: 1px solid #6d7a8a;
            border-right: 1px solid #6d7a8a;
            border-bottom: 4px solid #3c80d0;
        }
    )";

    // border style
    QString borderStyle = R"(
        QPushButton {
            border-bottom: 4px solid #3cf0ff;
        }
    )";

    // select group
    auto applyStyle = [&](const QList<QPushButton*>& group) {
        for (auto b : group) {
            if (b == btn)
                b->setStyleSheet(baseStyle + borderStyle);
            else
                b->setStyleSheet(baseStyle);
        }
    };

    // processing selected group
    if (controlButtons.contains(btn)) {
        applyStyle(controlButtons);
    } else if (mediaButtons.contains(btn)) {
        applyStyle(mediaButtons);
    }
}

// control_page slide effect
void MainWindow::slideToPage(int index)
{
    int current = ui->ControlWidget->currentIndex();
    if (current == index)
        return;

    QWidget* nextWidget = ui->ControlWidget->widget(index);

    int w = ui->ControlWidget->width();
    int h = ui->ControlWidget->height();

    // next 페이지를 오른쪽 바깥에 배치
    nextWidget->setGeometry(w, 0, w, h);
    nextWidget->show();

    // 애니메이션 객체 설정
    QPropertyAnimation* anim = new QPropertyAnimation(nextWidget, "geometry");
    anim->setDuration(300);
    anim->setStartValue(QRect(w, 0, w, h));
    anim->setEndValue(QRect(0, 0, w, h));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // 인덱스 변경
    ui->ControlWidget->setCurrentIndex(index);
}

MainWindow::~MainWindow()
{
    if (m_speedAlertPlayer) {
        m_speedAlertPlayer->stop();
        delete m_speedAlertPlayer;
        m_speedAlertPlayer = nullptr;
    }
    delete ui;
}

void MainWindow::on_pushButton_Play_Pause_clicked()
{
    handlePlayPause(this);
}

void MainWindow::on_pushButton_Stop_clicked()
{
    handleStop(this);
}

void MainWindow::on_pushButton_Volume_clicked()
{
    handleVolumeToggle(this);
}

void MainWindow::on_horizontalSlider_Volume_valueChanged(int value)
{
    handleVolumeChanged(this, value);
}

void MainWindow::on_pushButton_Seek_Backward_clicked()
{
    handleSeekBackward(this);
}

void MainWindow::on_pushButton_Seek_Forward_clicked()
{
    handleSeekForward(this);
}

// mouse ripple effect
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
//    if (ripple)
//        ripple->startRipple(event->pos());
}

void MainWindow::on_pushButton_user_clicked()
{
    // 계정 선택 페이지를 열고, 선택되면 현재 유저 세팅
    auto sel = new AccountSelectPage();
    sel->setAttribute(Qt::WA_DeleteOnClose);
    sel->showFullScreen();   // or sel->show();

    connect(sel, &AccountSelectPage::accountSelected,
            this, [this, sel](int id, const QString& /*name*/) {
                this->setCurrentUser(id);  // 아이콘/상태 즉시 갱신
                sel->close();
            });
}

// helper : smart mode debug
static inline void logWrite(const char* field, int oldv, int newv) {
    qDebug() << "[SHM]" << field << ":" << oldv << "->" << newv;
}

static inline void cpy16(char dst[16], const char* src) {
    if (!src) { dst[0] = '\0'; return; }
    std::strncpy(dst, src, 15);
    dst[15] = '\0';
}

static inline int clampTemp(int t)        { return qBound(16, t, 30); }
static inline int clampWallpaper(int idx) { return qBound(0,  idx, 5); }

// smart mode state
void MainWindow::dumpUserState(const char* tag)
{
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    const user_state_t& u = shm_ptr->user;
    qDebug().nospace()
        << "[SHM] dump" << (tag ? " (" + QString::fromUtf8(tag) + ")" : "")
        << " auto{aircon=" << u.aircon_autoflag
        << ", ambient="    << u.ambient_autoflag
        << ", window="     << u.window_autoflag
        << ", wiper="      << u.wiper_autoflag
        << ", wallpaper="  << u.wallpaper_flag << "}"
        << " val{aircon="  << u.aircon_val
        << ", ambient_color=" << u.ambient_color
        << ", wallpaper_num=" << u.wallpaper_num << "}";
}

// user_state_t 세터들
void MainWindow::setUserAirconAuto(int on) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    on = on ? 1 : 0; // 정규화
    int oldv = shm_ptr->user.aircon_autoflag;
    if (oldv == on) return; // (옵션) 변화 없으면 skip
    shm_ptr->user.aircon_autoflag = on;
    logWrite("user.aircon_autoflag", oldv, on);
}

void MainWindow::setUserAirconTemp(int t) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    t = clampTemp(t); // 안전 범위
    int oldv = shm_ptr->user.aircon_val;
    if (oldv == t) return; // (옵션)
    shm_ptr->user.aircon_val = t;
    logWrite("user.aircon_val", oldv, t);
}

void MainWindow::setUserAmbientAuto(int on) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    on = on ? 1 : 0;
    int oldv = shm_ptr->user.ambient_autoflag;
    if (oldv == on) return; // (옵션)
    shm_ptr->user.ambient_autoflag = on;
    logWrite("user.ambient_autoflag", oldv, on);
}

void MainWindow::setUserAmbientColorName(const char* name) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    char oldv[16]; cpy16(oldv, shm_ptr->user.ambient_color);

    // (옵션) 정책 통일: 소문자/대문자 선택
    char tmp[16]; cpy16(tmp, name);
    // for (int i=0; tmp[i]; ++i) tmp[i] = std::toupper(static_cast<unsigned char>(tmp[i])); // 대문자 원하면 사용

    if (std::strncmp(oldv, tmp, 16) == 0) return; // (옵션)
    cpy16(shm_ptr->user.ambient_color, tmp);
    qDebug() << "[SHM] user.ambient_color:" << oldv << "->" << shm_ptr->user.ambient_color;
}

void MainWindow::setUserWindowAuto(int on) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    on = on ? 1 : 0;
    int oldv = shm_ptr->user.window_autoflag;
    if (oldv == on) return; // (옵션)
    shm_ptr->user.window_autoflag = on;
    logWrite("user.window_autoflag", oldv, on);
}

void MainWindow::setUserWiperAuto(int on) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    on = on ? 1 : 0;
    int oldv = shm_ptr->user.wiper_autoflag;
    if (oldv == on) return; // (옵션)
    shm_ptr->user.wiper_autoflag = on;
    logWrite("user.wiper_autoflag", oldv, on);
}

void MainWindow::setUserWallpaperFlag(int on) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    on = on ? 1 : 0;
    int oldv = shm_ptr->user.wallpaper_flag;
    if (oldv == on) return; // (옵션)
    shm_ptr->user.wallpaper_flag = on;
    logWrite("user.wallpaper_flag", oldv, on);
}

void MainWindow::setUserWallpaperNum(int idx) {
    if (!shm_ptr) { qWarning() << "[SHM] not initialized"; return; }
    idx = clampWallpaper(idx); // 0~5
    int oldv = shm_ptr->user.wallpaper_num;
    if (oldv == idx) return; // (옵션)
    shm_ptr->user.wallpaper_num = idx;
    logWrite("user.wallpaper_num", oldv, idx);
}


// Naviiii
// ========== node parser ==========
QVector<Node> MainWindow::parseNodesFromCsv(const QString &filename)
{
    QVector<Node> nodes;
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open nodes csv:" << filename;
        return nodes;
    }
    // 도로 정보 추가 후 
    QTextStream in(&file);
    QString headerLine = in.readLine();  // 헤더 건너뛰기

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(',');

        // 최소 3개 컬럼 필요: osmid,y,x
        if (parts.size() < 3) continue;

        bool okId, okLat, okLon;
        qint64 id = parts[0].toLongLong(&okId);
        double lat = parts[1].toDouble(&okLat);
        double lon = parts[2].toDouble(&okLon);

        if (okId && okLat && okLon) {
            Node node;
            node.id = id;
            node.lat = lat;
            node.lon = lon;
            nodes.append(node);
        }
    }
    //qDebug() << "parseNodesFromCsv: loaded node count =" << nodes.size();
    return nodes;
}

QStringList parseCsvLine(const QString &line) {
    QStringList result;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line[i];

        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            result.append(field.trimmed());
            field.clear();
        } else {
            field.append(c);
        }
    }

    result.append(field.trimmed()); // 마지막 필드
    return result;
}


// =========== way parser ===========
QVector<Way> MainWindow::parseWaysFromCsv(const QString &filename)
{
    QVector<Way> ways;
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open ways csv:" << filename;
        return ways;
    }

    // 도로 정보 추가 후
    QTextStream in(&file);
    QString headerLine = in.readLine();  // 헤더 건너뛰기

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList parts = parseCsvLine(line);

        if (parts.size() < 2) continue;

        bool okU, okV;
        qint64 u = parts[0].toLongLong(&okU);
        qint64 v = parts[1].toLongLong(&okV);
        if (!(okU && okV)) continue;

        Way way;
        way.node_ids.clear();
        way.node_ids.append(u);
        way.node_ids.append(v);

        // highway 타입 (3번째 컬럼)
        if (parts.size() > 2)
            way.highway_type = parts[2].trimmed();

        // name (4번째 컬럼)
        if (parts.size() > 3)
            way.name = parts[3].trimmed();

        // maxspeed
        if (parts.size() > 5) {
            QString speedStr = parts[5].trimmed();
            // if (speedStr != "") qDebug() << "maxspeed raw field:" << speedStr;
            bool okSpeed = false;
            int speed = speedStr.toInt(&okSpeed);
            way.maxspeed = okSpeed ? speed : -1;
        } else {
            way.maxspeed = -1;
        }

        // tunnel
        if (parts.size() > 6)  // tunnel 필드는 7번째(6번) 컬럼
        {
            way.tunnel = parts[6].trimmed();
            //if (way.tunnel != "") qDebug() << way.tunnel;
        }

        ways.append(way);
    }

    //qDebug() << "parseWaysFromCsv: loaded way count =" << ways.size() << "ways from" << filename;
    return ways;
}

int MainWindow::getCurrentSpeedLimit(qint64 nodeId)
{
    int speedLimit = -1;
    for (const Way &way : m_ways) {
        if (way.node_ids.contains(nodeId)) {
            //qDebug() << nodeId << "maxspeed =" << way.maxspeed;
            if (way.maxspeed > 0) {
                if (speedLimit < 0 || way.maxspeed < speedLimit) // 가장 낮은 제한속도 선택 (안전하게)
                    speedLimit = way.maxspeed;
            }
        }
    }
    return speedLimit;
}

// tunnel edge
QSet<QPair<qint64, qint64>> MainWindow::extractTunnelEdges(const QVector<Way>& ways) {
    QSet<QPair<qint64, qint64>> Edges;
    for (const Way& way : ways) {
        QString tunnelValue = way.tunnel.trimmed().toLower();
        if (tunnelValue == "yes") {
            if (way.node_ids.size() == 2) {
                m_tunnelEdges.insert(qMakePair(way.node_ids[0], way.node_ids[1]));
            }
            // node_ids가 여러 개면 연속쌍 처리!
        }
    }
    return m_tunnelEdges;
}

void MainWindow::loadMapData()
{
    if (!m_mapView) return;
    m_nodes = parseNodesFromCsv("/run/media/mmcblk1p1/merged_nodes.csv");
    m_ways = parseWaysFromCsv("/run/media/mmcblk1p1/merged_ways.csv");

    m_tunnelEdges = extractTunnelEdges(m_ways);

    qDebug() << "[디버깅] nodes loaded =" << m_nodes.size();
    qDebug() << "[디버깅] ways loaded =" << m_ways.size();

    m_mapView->setMapData(m_nodes, m_ways);

    if (!m_nodes.isEmpty())
        m_mapView->setCurrentLocation(37.4777, 126.8805);
}

// 소리 경고
void MainWindow::playSpeedAlert()
{
    if (!m_speedAlertPlayer) {
        m_speedAlertPlayer = new QMediaPlayer(this);
        m_speedAlertPlayer->setMedia(QUrl("qrc:/sounds/warning.wav"));  // 경고음 경로 (리소스 파일 등록 권장)
        m_speedAlertPlayer->setVolume(100);
    }

    if (!m_speedAlertOn) {
        m_speedAlertPlayer->play();
        m_speedAlertOn = true;
    }
}

void MainWindow::stopSpeedAlert()
{
    if (m_speedAlertPlayer && m_speedAlertPlayer->state() == QMediaPlayer::PlayingState) {
        m_speedAlertPlayer->stop();
    }
    m_speedAlertOn = false;
}


// 제한속도 팝업 및 경고 처리 함수
void MainWindow::handleSpeedLimitUI(int curSpeedLimit, int currentSpeed)
{
    if (curSpeedLimit <= 0) {
        if (m_speedLimitPopup) {
            m_speedLimitPopup->close();
            delete m_speedLimitPopup;
            m_speedLimitPopup = nullptr;
        }
        m_prevSpeedLimit = -1;
        stopSpeedAlert(); // ⬅ 경고음 중지
        if (speedLimitLabel) speedLimitLabel->hide();
        return;
    }

    if (curSpeedLimit != m_prevSpeedLimit) {
        if (m_speedLimitPopup) {
            m_speedLimitPopup->close();
            delete m_speedLimitPopup;
            m_speedLimitPopup = nullptr;
        }
        m_speedLimitPopup = new QMessageBox(QMessageBox::Information,
                                            "제한속도 안내",
                                            QString("제한속도: %1 km/h").arg(curSpeedLimit),
                                            QMessageBox::NoButton, this);
        m_speedLimitPopup->setModal(false);
        m_speedLimitPopup->show();
        m_speedLimitPopup->raise();

        m_prevSpeedLimit = curSpeedLimit;
    }

    // 과속 상태
    if (currentSpeed > curSpeedLimit) {
        playSpeedAlert(); // ⬅ QMediaPlayer 버전 호출
        if (speedLimitLabel) {
            speedLimitLabel->setStyleSheet("color: red; font-weight: bold;");
            speedLimitLabel->setText(QString("속도: %1 km/h (과속)").arg(currentSpeed));
            speedLimitLabel->show();
            speedLimitLabel->raise();
        }
    }
    // 정상 속도 상태
    else {
        stopSpeedAlert();
        if (speedLimitLabel) {
            speedLimitLabel->setStyleSheet("color: black;");
            speedLimitLabel->setText(QString("속도: %1 km/h").arg(currentSpeed));
            speedLimitLabel->show();
            speedLimitLabel->raise();
        }
    }
}

// 터널 진입/이탈 경고 처리 함수
void MainWindow::handleTunnelAlert(bool isTunnelNow, const QString& wayName, qint64 prevNid, qint64 currNid, double wayLength)
{
    if (isTunnelNow && !m_inTunnel) {
        if (m_tunnelPopup) {
            delete m_tunnelPopup;
            m_tunnelPopup = nullptr;
        }
        m_tunnelPopup = new QMessageBox(QMessageBox::Information, "터널 진입",
                                       "터널 구간에 진입했습니다.",
                                       QMessageBox::NoButton, this);
        m_tunnelPopup->setModal(false);
        m_tunnelPopup->show();
        m_tunnelPopup->raise();
        m_inTunnel = true;

        qDebug().noquote() << QString("[터널 진입] %1 -> %2, %3, %4 m")
                                .arg(prevNid).arg(currNid)
                                .arg(wayName.isEmpty() ? "-" : wayName)
                                .arg(wayLength, 0, 'f', 2);
    }
    else if (!isTunnelNow && m_inTunnel) {
        if (m_tunnelPopup) {
            m_tunnelPopup->close();
            delete m_tunnelPopup;
            m_tunnelPopup = nullptr;
        }
        m_inTunnel = false;

        qDebug().noquote() << QString("[터널 종료] %1 -> %2").arg(prevNid).arg(currNid);
    }
}



// 시뮬레이션 함수
// 터널/속도제한 연속 경고 수정
bool m_inTunnel = false;
int m_prevSpeedLimit = -1;

double m_travelledOnSegment = 0.0; // 속도 반영

#if 0
void MainWindow::startRouteSimulation()
{
    // 라벨 없으면 생성
    if (!speedLimitLabel) {
        speedLimitLabel = new QLabel(this);   // 부모는 MainWindow
        speedLimitLabel->setGeometry(20, 20, 250, 40); // 위치와 크기
        speedLimitLabel->setAlignment(Qt::AlignCenter);
        speedLimitLabel->setStyleSheet(
            "QLabel { background-color: white; color: black; "
            "padding: 4px 8px; font-size: 100px; font-weight: bold; "
            "border: 1px solid gray; border-radius: 5px; }"
        );
        speedLimitLabel->hide(); // 초기에는 숨김
    }

    const QVector<qint64> &routeIds = m_mapView->getRouteNodeIds();
    if (routeIds.size() < 2) return;

    if (m_simulateTimer) {
        m_simulateTimer->stop();
        delete m_simulateTimer;
    }
    m_currentRouteIdx = 0;
    m_travelledOnSegment = 0.0;

    m_simulateTimer = new QTimer(this);
    qDebug() << "[Sim] 시뮬레이션 시작 - 경로 노드 수:" << routeIds.size();

    // 경로 구간 거리 계산 - 하버사인
    auto calculateDistance = [](const Node* a, const Node* b) -> double {
        if (!a || !b) return 0.0;
        constexpr double R = 6371000.0;
        double lat1 = a->lat * M_PI / 180.0;
        double lat2 = b->lat * M_PI / 180.0;
        double dLat = (b->lat - a->lat) * M_PI / 180.0;
        double dLon = (b->lon - a->lon) * M_PI / 180.0;
        double h = sin(dLat/2) * sin(dLat/2) +
                   cos(lat1) * cos(lat2) *
                   sin(dLon/2) * sin(dLon/2);
        double c = 2 * atan2(sqrt(h), sqrt(1-h));
        return R * c;
    };

    connect(m_simulateTimer, &QTimer::timeout, this, [=]() mutable {
        if (!shm_ptr) return;
        
        if (m_currentRouteIdx >= routeIds.size()) {
            m_simulateTimer->stop();
            qDebug() << "[Sim] 목적지 도착";
            return;
        }

        // ---- 속도 기반 보간 이동 ----
        double speed_kmh = shm_ptr->speed;        // CAN 실시간 속도
        double speed_mps = speed_kmh / 3.6;       // m/s
        double deltaT    = 0.05;                  // 50ms
        double moveDist  = speed_mps * deltaT;    // 이번 tick 이동(m)

        const Node *currNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx]);
        const Node *nextNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx+1]);
        if (!currNode || !nextNode) return;

        double segLen = calculateDistance(currNode, nextNode);
        m_travelledOnSegment += moveDist;

        // 구간을 다 채웠으면 다음 구간으로
        while (m_travelledOnSegment >= segLen && m_currentRouteIdx < routeIds.size()-2) {
            m_currentRouteIdx++;
            m_travelledOnSegment -= segLen;
            currNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx]);
            nextNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx+1]);
            segLen   = calculateDistance(currNode, nextNode);
        }

        // 현재 구간 내 위치 비율
        double ratio = m_travelledOnSegment / segLen;
        double lat = currNode->lat + (nextNode->lat - currNode->lat) * ratio;
        double lon = currNode->lon + (nextNode->lon - currNode->lon) * ratio;

        // 현재 구간 정보
        int idx = m_currentRouteIdx;
        qint64 currNodeId = routeIds[m_currentRouteIdx];
        //qint64 curr_nid = routeIds[idx];
        qint64 prev_nid = (idx > 0) ? routeIds[idx - 1] : -1;
        const Node* currNode = m_mapView->getNodeById(currNodeId);

        //const Node* node = m_mapView->getNodeById(curr_nid);

        if (!currNode) {
            qDebug() << "[Sim] 현재 노드 없음" << currNodeId;
            m_currentRouteIdx++;
            return;
        }
        int curSpeedLimit = -1;
        bool isTunnelNow = false;
        QString wayName;
        double wayLength = 0.0;

        if (idx > 0) {
            for (const Way& way : m_ways) {
                if (way.node_ids.size() == 2 &&
                   ((way.node_ids[0] == prev_nid && way.node_ids[1] == curr_nid) ||
                    (way.node_ids[1] == prev_nid && way.node_ids[0] == curr_nid))) {

                    if (way.maxspeed > 0) curSpeedLimit = way.maxspeed;
                    if (way.tunnel.compare("yes", Qt::CaseInsensitive) == 0)
                        isTunnelNow = true;
                    wayName = way.name;
                    wayLength = calculateDistance(
                        m_mapView->getNodeById(way.node_ids[0]),
                        m_mapView->getNodeById(way.node_ids[1])
                    );
                    break;
                }
            }
        }

        // --- 터널 팝업 제어 ---
        if (isTunnelNow && !m_inTunnel) {
            if (m_tunnelPopup) { delete m_tunnelPopup; m_tunnelPopup = nullptr; }
            m_tunnelPopup = new QMessageBox(QMessageBox::Information, "터널 진입",
                                            "터널 구간에 진입했습니다.",
                                            QMessageBox::NoButton, this);
            m_tunnelPopup->setModal(false);
            m_tunnelPopup->show();
            m_inTunnel = true;

            qDebug().noquote() << QString("[터널 진입] %1 -> %2, %3, %4 m")
                                 .arg(prev_nid).arg(curr_nid)
                                 .arg(wayName.isEmpty() ? "-" : wayName)
                                 .arg(wayLength, 0, 'f', 2);
        }
        else if (!isTunnelNow && m_inTunnel) {
            if (m_tunnelPopup) { m_tunnelPopup->close(); delete m_tunnelPopup; m_tunnelPopup = nullptr; }
            m_inTunnel = false;
            qDebug().noquote() << QString("[터널 종료] %1 -> %2").arg(prev_nid).arg(curr_nid);
        }

        // --- 속도제한 팝업 제어 ---
        if (curSpeedLimit > 0) {
            if (curSpeedLimit != m_prevSpeedLimit) {
                // 속도제한 진입 
                if (m_speedLimitPopup) { m_speedLimitPopup->close(); delete m_speedLimitPopup; m_speedLimitPopup = nullptr; }
                m_speedLimitPopup = new QMessageBox(QMessageBox::Information,
                                                    "제한속도 안내",
                                                    QString("제한속도: %1 km/h").arg(curSpeedLimit),
                                                    QMessageBox::NoButton, this);
                m_speedLimitPopup->setModal(false);
                m_speedLimitPopup->show();
                QTimer::singleShot(2000, m_speedLimitPopup, [this]() {
                    if (m_speedLimitPopup) m_speedLimitPopup->close();
                });

                m_prevSpeedLimit = curSpeedLimit;
                qDebug().noquote() << QString("[== 진입 ==] %1 km/h | %2 -> %3 | %4 | %5 m")
                                     .arg(curSpeedLimit).arg(prev_nid).arg(curr_nid)
                                     .arg(wayName.isEmpty() ? "-" : wayName)
                                     .arg(wayLength, 0, 'f', 2);
            }
            else {
                // 속도제한구간
                qDebug().noquote() << QString("[유지] %1 km/h | 구간 노드: %2 -> %3 | 도로명: %4")
               .arg(m_prevSpeedLimit).arg(prev_nid).arg(curr_nid)
               .arg(wayName.isEmpty() ? "-" : wayName);
            }
            // UI Label 항상 갱신
            // 🔹 라벨 업데이트
            speedLimitLabel->setText(
                QString("제한속도: %1 km/h\n")
                    .arg(m_prevSpeedLimit)
            );
            speedLimitLabel->show();
            speedLimitLabel->raise();
   
        }
        else if (m_prevSpeedLimit > 0) {
            // 속도제한 해제
            if (m_speedLimitPopup) { m_speedLimitPopup->close(); delete m_speedLimitPopup; m_speedLimitPopup = nullptr; }
            qDebug().noquote() << QString("[== 해제 ==] %1 -> %2").arg(prev_nid).arg(curr_nid);
            m_prevSpeedLimit = -1;
        }

        // --- 위치 이동 ---
        // 진행방향 계산
        double heading = calculateBearing(currNode->lat, currNode->lon,
                                          nextNode->lat, nextNode->lon);
        m_mapView->setCurrentHeading(heading);

        // 지도 업데이트
        m_mapView->setCurrentLocation(lat, lon);
        m_mapView->setCurrentRouteIndex(m_currentRouteIdx);

#if 0
        // 1) 진행 방향 계산 (다음 노드가 있으면)
        if (nextNode) {
            double heading = calculateBearing(currNode->lat, currNode->lon,
                                              nextNode->lat, nextNode->lon);
            m_mapView->setCurrentHeading(heading);
        }
        m_mapView->setCurrentLocation(currNode->lat, currNode->lon);
        m_mapView->setCurrentRouteIndex(m_currentRouteIdx);

        m_currentRouteIdx++;
#endif
    });

    m_simulateTimer->start(50);
}
#endif

void MainWindow::startRouteSimulation()
{
    // 제한속도 라벨 생성
    if (!speedLimitLabel) {
        speedLimitLabel = new QLabel(this);
        speedLimitLabel->setGeometry(20, 20, 250, 40);
        speedLimitLabel->setAlignment(Qt::AlignCenter);
        speedLimitLabel->setStyleSheet(
            "QLabel { background-color: white; color: black; "
            "padding: 4px 8px; font-size: 100px; font-weight: bold; "
            "border: 1px solid gray; border-radius: 5px; }"
        );
        speedLimitLabel->hide();
    }

    const auto &routeIds = m_mapView->getRouteNodeIds();
    if (routeIds.size() < 2) return;

    if (m_simulateTimer) {
        m_simulateTimer->stop();
        delete m_simulateTimer;
    }

    m_currentRouteIdx = 0;
    m_travelledOnSegment = 0.0;

    // 거리 계산
    auto calculateDistance = [](const Node* a, const Node* b) -> double {
        if (!a || !b) return 0.0;
        constexpr double R = 6371000.0;
        double lat1 = a->lat * M_PI / 180.0;
        double lat2 = b->lat * M_PI / 180.0;
        double dLat = (b->lat - a->lat) * M_PI / 180.0;
        double dLon = (b->lon - a->lon) * M_PI / 180.0;
        double h = sin(dLat/2) * sin(dLat/2) +
                   cos(lat1) * cos(lat2) * sin(dLon/2) * sin(dLon/2);
        double c = 2 * atan2(sqrt(h), sqrt(1 - h));
        return R * c;
    };

    m_simulateTimer = new QTimer(this);
    connect(m_simulateTimer, &QTimer::timeout, this, [=]() mutable {
        if (!shm_ptr) return;
        if (m_currentRouteIdx >= routeIds.size()) {
            m_simulateTimer->stop();
            qDebug() << "[Sim] 목적지 도착";
            return;
        }

        // 속도 기반 위치 이동
        double speed_kmh = shm_ptr->speed;
        double speed_mps = speed_kmh / 3.6;
        constexpr double deltaT = 0.05;
        double moveDist = speed_mps * deltaT;

        const Node* currNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx]);
        const Node* nextNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx + 1]);
        if (!currNode || !nextNode) return;

        double segLen = calculateDistance(currNode, nextNode);
        m_travelledOnSegment += moveDist;

        while (m_travelledOnSegment >= segLen && m_currentRouteIdx < routeIds.size() - 2) {
            m_currentRouteIdx++;
            m_travelledOnSegment -= segLen;
            currNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx]);
            nextNode = m_mapView->getNodeById(routeIds[m_currentRouteIdx + 1]);
            segLen = calculateDistance(currNode, nextNode);
        }

        double ratio = m_travelledOnSegment / segLen;
        double lat = currNode->lat + (nextNode->lat - currNode->lat) * ratio;
        double lon = currNode->lon + (nextNode->lon - currNode->lon) * ratio;

        // 제한속도/터널 정보 분석
        int idx = m_currentRouteIdx;
        qint64 currNodeId = routeIds[idx];
        qint64 prevNid = (idx > 0) ? routeIds[idx - 1] : -1;

        int curSpeedLimit = -1;
        bool isTunnelNow = false;
        QString wayName;
        double wayLength = 0.0;

        if (idx > 0) {
            for (const Way& way : m_ways) {
                if (way.node_ids.size() == 2 &&
                    ((way.node_ids[0] == prevNid && way.node_ids[1] == currNodeId) ||
                     (way.node_ids[1] == prevNid && way.node_ids[0] == currNodeId))) {

                    if (way.maxspeed > 0) curSpeedLimit = way.maxspeed;
                    if (way.tunnel.compare("yes", Qt::CaseInsensitive) == 0)
                        isTunnelNow = true;
                    wayName = way.name;
                    wayLength = calculateDistance(
                        m_mapView->getNodeById(way.node_ids[0]),
                        m_mapView->getNodeById(way.node_ids[1])
                    );
                    break;
                }
            }
        }

        // 팝업/UI 함수 호출
        handleTunnelAlert(isTunnelNow, wayName, prevNid, currNodeId, wayLength);
        handleSpeedLimitUI(curSpeedLimit, static_cast<int>(speed_kmh));

        // 지도 위치 업데이트
        double heading = calculateBearing(currNode->lat, currNode->lon,
                                          nextNode->lat, nextNode->lon);
        m_mapView->setCurrentHeading(heading);
        m_mapView->setCurrentLocation(lat, lon);
        m_mapView->setCurrentRouteIndex(m_currentRouteIdx);
    });

    m_simulateTimer->start(50);
}
