#include "mainwindow.h"                         // class definition
#include "ui_mainwindow.h"                      // Access UI Member

#include "sensor_handler.h"                     // RealTime Sensor

#include "aircon_page.h"                        // Aircon
#include "ambient_page.h"                       // Ambient
#include "window_page.h"                        // window
#include "headlamp_page.h"                      // Headlamp
#include "trunk_page.h"                         // Trunk
#include "video_page.h"                         // Video
#include "enter.h"                              // Enter

#include <QGraphicsDropShadowEffect>            // Shadow Effect
#include <QTimer>                               // Timer
#include <QTime>                                // Clock
#include <QDate>                                // Date

#include <QResizeEvent>                         // resizeEvent

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

// FIFO
#define PATH_FIFO "/tmp/qt_fifo"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_timer(new QTimer(this))
{
    ui->setupUi(this);

    ripple = new RippleWidget(this);
    ripple->resize(this->size());
    ripple->raise();

    this->setGeometry(0, 0, 2560, 1600);                    // static size (x, y, width, height)
    ui->centralwidget->setFixedSize(2560, 1600);            // static size (width, height)

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);         // 테두리 제거
    this->show();
    // this->showFullScreen();  // 전체 화면으로 시작
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

    // init video_module
    initVideoConnections(this);

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

    // 처리 안 된 이벤트는 기본 처리
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);  // 기본 이벤트 유지

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
    ui->btn_Enter->setIconSize(ui->btn_Enter->size() * 0.8);
    ui->btn_Enter->setText("");
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
    if (!fifo.open(QIODevice::WriteOnly | QIODevice::Text))
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
    slideToPage(1);
}

void MainWindow::on_btn_ambient_clicked()
{
    qDebug() << "[INFO] Ambient button clicked!";
    highlightOnly(ui->btn_ambient);
    slideToPage(2);
}

void MainWindow::on_btn_window_clicked()
{
    qDebug() << "[INFO] Window button clicked!";
    highlightOnly(ui->btn_window);
    slideToPage(3);
}

void MainWindow::on_btn_headlamp_clicked()
{
    qDebug() << "[INFO] Head Lamp button clicked!";
    highlightOnly(ui->btn_headlamp);
    slideToPage(4);
}

void MainWindow::on_btn_music_clicked()
{
    qDebug() << "[INFO] Music button clicked!";
    highlightOnly(ui->btn_music);
    ui->stackedWidget_2->setCurrentIndex(0);  // page_music
}

void MainWindow::on_btn_video_clicked()
{
    qDebug() << "[INFO] Video button clicked!";
    highlightOnly(ui->btn_video);
    ui->stackedWidget_2->setCurrentIndex(1);  // page_video
}

void MainWindow::on_btn_trunk_clicked()
{
    qDebug() << "[INFO] Trunk button clicked!";
    highlightOnly(ui->btn_trunk);
    ui->stackedWidget_2->setCurrentIndex(2);  // page_trunk
}

void MainWindow::on_btn_bluetooth_clicked()
{
    qDebug() << "[INFO] Bluetooth button clicked!";
    highlightOnly(ui->btn_bluetooth);
    slideToPage(5);
}

void MainWindow::on_btn_settings_clicked()
{
    qDebug() << "[INFO] Settings button clicked!";
    highlightOnly(ui->btn_settings);
    slideToPage(6);
}

void MainWindow::on_btn_Navi_clicked()
{
    qDebug() << "[INFO] Navi button clicked!";
    highlightOnly(ui->btn_Navi);
    ui->stackedWidget_2->setCurrentIndex(3);  // page_navi
}

void MainWindow::on_btn_Enter_clicked()
{
    qDebug() << "[INFO] Enter button clicked!";
    highlightOnly(ui->btn_Enter);
    ui->stackedWidget_2->setCurrentIndex(4);  // page_enter
}

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
        ui->btn_Enter
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

    // border
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

void MainWindow::slideToPage(int index)
{
    int current = ui->stackedWidget->currentIndex();
    if (current == index)
        return;

    QWidget* nextWidget = ui->stackedWidget->widget(index);

    int w = ui->stackedWidget->width();
    int h = ui->stackedWidget->height();

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
    ui->stackedWidget->setCurrentIndex(index);
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

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (ripple)
        ripple->startRipple(event->pos());
}
