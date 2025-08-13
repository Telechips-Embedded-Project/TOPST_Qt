#include "video_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "ripplewidget.h"

#include <QTime>

static RippleWidget* videoRipple = nullptr;

static QMediaPlayer* player = nullptr;
static QVideoWidget* videoWidget = nullptr;
static bool isPaused = true;
static bool isMuted = false;
static qint64 totalDuration = 0;

QVideoWidget* getVideoWidget() { return videoWidget; }

bool startVideoRippleAtGlobal(MainWindow* /*w*/, const QPoint& globalPos)
{
    if (!videoWidget || !videoRipple) return false;

    // global → videoWidget 좌표
    QPoint p = videoWidget->mapFromGlobal(globalPos);

    // 비디오 영역 안일 때만 실행
    if (!QRect(QPoint(0,0), videoWidget->size()).contains(p)) return false;

    videoRipple->startRipple(p);
    return true;
}

void initVideoConnections(MainWindow* w)
{
    auto ui = w->getUi();

    player = new QMediaPlayer(w);

    ui->pushButton_Play_Pause->setIcon(w->style()->standardIcon(QStyle::SP_MediaPlay));
    ui->pushButton_Stop->setIcon(w->style()->standardIcon(QStyle::SP_MediaStop));
    ui->pushButton_Seek_Backward->setIcon(w->style()->standardIcon(QStyle::SP_MediaSeekBackward));
    ui->pushButton_Seek_Forward->setIcon(w->style()->standardIcon(QStyle::SP_MediaSeekForward));
    ui->pushButton_Volume->setIcon(w->style()->standardIcon(QStyle::SP_MediaVolume));

    ui->horizontalSlider_Volume->setRange(0, 100);
    ui->horizontalSlider_Volume->setValue(30);
    player->setVolume(30);

    QObject::connect(player, &QMediaPlayer::durationChanged, [=](qint64 dur) {
        totalDuration = dur / 1000;
        ui->horizontalSlider_Duration->setMaximum(totalDuration);
    });

    QObject::connect(player, &QMediaPlayer::positionChanged, [=](qint64 pos) {
        if (!ui->horizontalSlider_Duration->isSliderDown())
            ui->horizontalSlider_Duration->setValue(pos / 1000);

        updateVideoPosition(w, pos / 1000);
    });

    QObject::connect(ui->listWidget_VideoList, &QListWidget::itemDoubleClicked,
                     [=](QListWidgetItem* item) { handleVideoSelection(w, item); });

    // 초기 영상 리스트 로딩
    loadVideoList(w, "./videos");
}

void loadVideoList(MainWindow* w, const QString& directory)
{
    auto ui = w->getUi();

    QDir dir(directory);
    QStringList videoFiles = dir.entryList(QStringList() << "*.mp4", QDir::Files);

    foreach (const QString &fileName, videoFiles) {
        QListWidgetItem* item = new QListWidgetItem(fileName);
        item->setData(Qt::UserRole, dir.absoluteFilePath(fileName));
        ui->listWidget_VideoList->addItem(item);
    }
}

void handleVideoSelection(MainWindow* w, QListWidgetItem* item)
{
    auto ui = w->getUi();
    QString path = item->data(Qt::UserRole).toString();

    if (!player || path.isEmpty()) {
        qDebug() << "[Video] Invalid player or path.";
        return;
    }

    if (player->mediaStatus() != QMediaPlayer::NoMedia) {
        player->stop();
        player->setMedia(QMediaContent());
    }

    // 기존 videoWidget 제거
    if (videoWidget) {
        videoWidget->hide();
        videoWidget->deleteLater();
        videoWidget = nullptr;
    }

    // 새 videoWidget 생성 및 설정
    videoWidget = new QVideoWidget(ui->groupBox_Video);
    videoWidget->setParent(ui->groupBox_Video);
    videoWidget->setGeometry(5, 5,
                             ui->groupBox_Video->width() - 10,
                             ui->groupBox_Video->height() - 10);
    videoWidget->show();

    videoWidget->installEventFilter(w);

    if (videoRipple) { videoRipple->deleteLater(); videoRipple = nullptr; }
    videoRipple = new RippleWidget(videoWidget);
    videoRipple->setGeometry(videoWidget->rect());
    videoRipple->raise();
    videoRipple->show();

    player->setVideoOutput(videoWidget);
    player->setMedia(QUrl::fromLocalFile(path));
    player->play();

    isPaused = false;
    ui->pushButton_Play_Pause->setIcon(w->style()->standardIcon(QStyle::SP_MediaPause));
}

void handlePlayPause(MainWindow* w)
{
    auto ui = w->getUi();

    if (!player) return;

    if (isPaused) {
        player->play();
        ui->pushButton_Play_Pause->setIcon(w->style()->standardIcon(QStyle::SP_MediaPause));
    } else {
        player->pause();
        ui->pushButton_Play_Pause->setIcon(w->style()->standardIcon(QStyle::SP_MediaPlay));
    }
    isPaused = !isPaused;
}

void handleStop(MainWindow* w)
{
    if (player) player->stop();
}

void handleVolumeToggle(MainWindow* w)
{
    auto ui = w->getUi();

    if (!player) return;

    isMuted = !isMuted;
    player->setMuted(isMuted);
    ui->pushButton_Volume->setIcon(
        w->style()->standardIcon(isMuted ? QStyle::SP_MediaVolumeMuted : QStyle::SP_MediaVolume));
}

void handleVolumeChanged(MainWindow* w, int value)
{
    if (player) player->setVolume(value);
}

void handleSeekBackward(MainWindow* w)
{
    auto ui = w->getUi();

    ui->horizontalSlider_Duration->setValue(ui->horizontalSlider_Duration->value() - 20);
    player->setPosition(ui->horizontalSlider_Duration->value() * 1000);
}

void handleSeekForward(MainWindow* w)
{
    auto ui = w->getUi();

    ui->horizontalSlider_Duration->setValue(ui->horizontalSlider_Duration->value() + 20);
    player->setPosition(ui->horizontalSlider_Duration->value() * 1000);
}

void updateVideoPosition(MainWindow* w, qint64 pos)
{
    auto ui = w->getUi();

    QTime cur((pos / 3600) % 60, (pos / 60) % 60, pos % 60);
    QTime tot((totalDuration / 3600) % 60, (totalDuration / 60) % 60, totalDuration % 60);

    QString format = totalDuration > 3600 ? "hh:mm:ss" : "mm:ss";

    ui->label_current_Time->setText(cur.toString(format));
    ui->label_Total_Time->setText(tot.toString(format));
}
