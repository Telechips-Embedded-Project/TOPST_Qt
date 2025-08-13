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

#include <QPainterPath>                         // for makeRounded

#include <QGraphicsDropShadowEffect>            // Shadow Effect
#include <QTimer>                               // Timer
#include <QTime>                                // Clock
#include <QDate>                                // Date

#include <QResizeEvent>                         // resizeEvent

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_timer(new QTimer(this))
{
    qputenv("QT_GSTREAMER_USE_OVERLAY", "0");

    ui->setupUi(this);

    ripple = new RippleWidget(this);
    ripple->resize(this->size());
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
    ui->btn_bluetooth->installEventFilter(this);
    ui->btn_settings->installEventFilter(this);
    ui->btn_Navi->installEventFilter(this);

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



    // init another_module....
    // ...

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

    // Test Trigger (Ctrl+Shift+N)
    auto sc = new QShortcut(QKeySequence("Ctrl+Shift+N"), this);
    connect(sc, &QShortcut::activated, this, [this]{
        m_banner->setMessage("Test: CO₂ 2500ppm 초과! 환기 시작");
        m_banner->showBanner(3500);  // for 3.5 sec
    });

    // user btn style
    ui->pushButton_user->setFixedSize(64, 64);
    ui->pushButton_user->setStyleSheet(
        "QPushButton { border: none; border-radius: 32px; background: transparent; }"
        "QPushButton:hover { opacity: 0.9; }"
    );

    // init value FAN Image(not gif)
    setAirconGif(this, ":/gif/aircon_fan.png", true);
}

void MainWindow::updateDateTime()
{
    QTime time = QTime::currentTime();
    QString timeStr = time.toString("h:mm AP");  // Time

    QDate date = QDate::currentDate();
    QString dateStr = date.toString("M월 d일 (ddd)");  // Date

    ui->label_time->setText(timeStr);
    ui->label_date->setText(dateStr);
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

    // Bluetooth btn
    else if (obj == ui->btn_bluetooth)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_bluetooth->setIcon(QIcon(":/images/Bluetooth_neon.png"));
            ui->btn_bluetooth->setIconSize(ui->btn_bluetooth->size() * 1);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_bluetooth->setIcon(QIcon(":/images/Bluetooth.png"));
            ui->btn_bluetooth->setIconSize(ui->btn_bluetooth->size() * scaleFactor);
            return true;
        }
    }

    // Settings btn
    else if (obj == ui->btn_settings)
    {
        if (event->type() == QEvent::Enter) {
            ui->btn_settings->setIcon(QIcon(":/images/Settings_neon.png"));
            ui->btn_settings->setIconSize(ui->btn_bluetooth->size() * 1.2);
            return true;
        }
        else if (event->type() == QEvent::Leave) {
            ui->btn_settings->setIcon(QIcon(":/images/Settings.png"));
            ui->btn_settings->setIconSize(ui->btn_bluetooth->size() * scaleFactor);
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

    // Bluetooth
    ui->btn_bluetooth->setIcon(QIcon(":/images/Bluetooth.png"));
    ui->btn_bluetooth->setIconSize(ui->btn_bluetooth->size() * scaleFactor);
    ui->btn_bluetooth->setText("");

    // Settings
    ui->btn_settings->setIcon(QIcon(":/images/Settings.png"));
    ui->btn_settings->setIconSize(ui->btn_settings->size() * scaleFactor);
    ui->btn_settings->setText("");

    // Navi
    ui->btn_Navi->setIcon(QIcon(":/images/Navi.png"));
    ui->btn_Navi->setIconSize(ui->btn_Navi->size() * 0.8);
    ui->btn_Navi->setText("");

    // Navi
//    ui->btn_Navi->setIcon(QIcon(":/images/Navi.png"));
    ui->btn_home->setIconSize(ui->btn_home->size() * 0.8);
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

void MainWindow::on_btn_bluetooth_clicked()
{
    qDebug() << "[INFO] Bluetooth button clicked!";
    highlightOnly(ui->btn_bluetooth);
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
        ui->btn_bluetooth,
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