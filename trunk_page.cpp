#include "trunk_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QLabel>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QImage>
#include <QPixmap>
#include <QDebug>

static QProcess*  s_camProc  = nullptr;
static QLabel*    s_camLabel = nullptr;
static QByteArray s_camBuf;

// SOI/EOI 마커
static const QByteArray SOI("\xFF\xD8", 2);
static const QByteArray EOI("\xFF\xD9", 2);

// widget_trunk_camera 안에 라벨/레이아웃 준비
static void ensureCameraWidget(MainWindow* w) {
    auto ui = w->getUi();
    QWidget* container = ui->widget_trunk_camera;
    if (!container->layout()) {
        auto lay = new QVBoxLayout(container);
        lay->setContentsMargins(0,0,0,0);
        container->setLayout(lay);
    }
    if (!s_camLabel) {
        s_camLabel = new QLabel(container);
        s_camLabel->setAlignment(Qt::AlignCenter);
        s_camLabel->setMinimumSize(160,120);
        s_camLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        s_camLabel->setStyleSheet("background:#000; color:#ccc;");
        container->layout()->addWidget(s_camLabel);
    }
}

// stdout에서 최신 JPEG 프레임만 뽑아 그리기
static void onCamStdout(MainWindow* /*w*/) {
    if (!s_camProc || !s_camLabel) return;
    s_camBuf.append(s_camProc->readAllStandardOutput());

    // 버퍼 과다 방지: 8MB 넘으면 마지막 SOI부터만 유지
    const int MAX_BUF = 8 * 1024 * 1024;
    if (s_camBuf.size() > MAX_BUF) {
        int idx = s_camBuf.lastIndexOf(SOI);
        if (idx > 0) s_camBuf.remove(0, idx);
        else s_camBuf.clear();
    }

    // 가능한 프레임을 모두 처리
    while (true) {
        int soi = s_camBuf.indexOf(SOI);
        if (soi < 0) break;
        if (soi > 0) s_camBuf.remove(0, soi); // 앞쪽 잡음 제거

        int eoi = s_camBuf.indexOf(EOI, 2);   // 버퍼 맨앞이 SOI이므로 2부터
        if (eoi < 0) break;                   // 아직 끝마커 미도착

        QByteArray frame = s_camBuf.left(eoi + 2);
        s_camBuf.remove(0, eoi + 2);

        QImage img;
        if (!img.loadFromData(frame, "JPG")) {
            static int n=0; if (n++ < 5)
                qWarning() << "[USB CAM] JPEG decode failed. size=" << frame.size()
                           << "head=" << frame.left(4).toHex();
            continue;
        }

        QPixmap pm = QPixmap::fromImage(img);
        if (pm.isNull()) {
            static int m=0; if (m++ < 5)
                qWarning() << "[USB CAM] pixmap is null after decode.";
            continue;
        }

        s_camLabel->setPixmap(pm.scaled(s_camLabel->size(),
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
        if (!s_camLabel->text().isEmpty())
            s_camLabel->clear();
    }
}

static void onCamStderr() {
    if (!s_camProc) return;
    auto err = s_camProc->readAllStandardError();
    if (!err.isEmpty())
        qDebug().noquote() << "[USB CAM stderr]" << QString::fromUtf8(err);
}

static void onCamFinished() {
    s_camBuf.clear();
    if (s_camLabel)
        s_camLabel->clear();
}

// ===== 외부 API =====
void trunkStart(MainWindow* w, const QString& device, int width, int height, int /*fps*/) {
    ensureCameraWidget(w);
    if (s_camProc && s_camProc->state() != QProcess::NotRunning)
        return;

    if (!s_camProc) {
        s_camProc = new QProcess(w);
        s_camProc->setProcessChannelMode(QProcess::SeparateChannels);

        // 람다로 static 핸들러 연결
        QObject::connect(s_camProc, &QProcess::readyReadStandardOutput,
                         [w](){ onCamStdout(w); });
        QObject::connect(s_camProc, &QProcess::readyReadStandardError,
                         [](){ onCamStderr(); });
        QObject::connect(s_camProc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                        w, [](int, QProcess::ExitStatus){ onCamFinished(); });
    }

    const QString cmd = QString(
        "v4l2-ctl -d %1 "
        "--set-fmt-video=width=%2,height=%3,pixelformat=MJPG "
        "--stream-mmap=3 --stream-count=0 --stream-to=-"
        /*
        "| gst-launch-1.0 -v fdsrc fd=0 ! "
        "image/jpeg,width=640,height=480,framerate=30/1 ! "
        "jpegparse ! jpegdec ! videoconvert ! waylandsink fullscreen=true sync=false"
        */
    ).arg(device).arg(width).arg(height);

    s_camBuf.clear();
    s_camProc->start("/bin/sh", {"-c", cmd});
    if (!s_camProc->waitForStarted(500)) {
        qWarning() << "[USB CAM] failed to start v4l2-ctl process.";
        return;
    }
    if (s_camLabel) s_camLabel->setText("Opening camera…");
}

void trunkStop(MainWindow* /*w*/) {
    if (s_camProc && s_camProc->state() != QProcess::NotRunning) {
        s_camProc->kill();
        s_camProc->waitForFinished(800);
    }
    s_camBuf.clear();
    if (s_camLabel) {
        s_camLabel->clear();
        s_camLabel->setText("Camera stopped");
    }
}

void initTrunkConnections(MainWindow* w) {
    auto ui = w->getUi();
    ensureCameraWidget(w);

    ui->pushButton_trunk_camera_on->setText("");
    ui->pushButton_trunk_camera_off->setText("");

    QObject::connect(ui->pushButton_trunk_camera_on,  &QPushButton::clicked, w, [w](){
        trunkStart(w, "/dev/video1", 1280, 720, 30);
        // 부하가 크면:
        // trunkStart(w, "/dev/video1", 640, 480, 15);

        // ON Button Style
        w->getUi()->pushButton_trunk_camera_on->setStyleSheet("border-image: url(:/images/ON_ON.png);");
        w->getUi()->pushButton_trunk_camera_off->setStyleSheet("border-image: url(:/images/OFF_OFF.png);");
    });
    QObject::connect(ui->pushButton_trunk_camera_off, &QPushButton::clicked, w, [w](){
        trunkStop(w);

        // OFF Button Style
        w->getUi()->pushButton_trunk_camera_on->setStyleSheet("border-image: url(:/images/ON_OFF.png);");
        w->getUi()->pushButton_trunk_camera_off->setStyleSheet("border-image: url(:/images/OFF_ON.png);");
    });

    QObject::connect(ui->EnterWidget, &QStackedWidget::currentChanged, w, [w](int idx){
        int trunkIdx = w->getUi()->EnterWidget->indexOf(w->getUi()->page_trunk);
        if (idx != trunkIdx) trunkStop(w);
    });
}
