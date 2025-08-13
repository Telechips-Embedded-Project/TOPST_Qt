#include "music_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMediaPlayer>
#include <QListWidgetItem>
#include <QDir>
#include <QFileInfo>
#include <QStyle>
#include <QDebug>

static QMediaPlayer* musicPlayer = nullptr;
static bool draggingPosition = false;
static bool isPausedMusic = false;
static bool isMutedMusic  = false;

static QString msToHMS(qint64 ms)
{
    if (ms < 0) ms = 0;
    qint64 secs = ms / 1000;
    int h = int(secs / 3600);
    int m = int((secs % 3600) / 60);
    int s = int(secs % 60);
    return (h > 0) ? QString("%1:%2:%3").arg(h,2,10,QChar('0'))
                                 .arg(m,2,10,QChar('0'))
                                 .arg(s,2,10,QChar('0'))
                   : QString("%1:%2").arg(m,2,10,QChar('0'))
                                      .arg(s,2,10,QChar('0'));
}

void initMusicConnections(MainWindow* w)
{
    auto ui = w->getUi();

    if (!musicPlayer) {
        musicPlayer = new QMediaPlayer(w);
        musicPlayer->setVolume(50);

        QObject::connect(musicPlayer, &QMediaPlayer::positionChanged,
                         w, [w](qint64 p){ updateMusicPosition(w, p); });

        QObject::connect(musicPlayer, &QMediaPlayer::durationChanged,
                         w, [w](qint64 d){ updateMusicDuration(w, d); });

        QObject::connect(musicPlayer, static_cast<void (QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            w, [](QMediaPlayer::Error e){ qWarning() << "[Music] player error:" << e; });
    }

    ui->horizontalSlider_Duration_2->setRange(0, 0);
    ui->horizontalSlider_Volume_2->setRange(0, 100);
    ui->horizontalSlider_Volume_2->setValue(musicPlayer->volume());
    ui->label_current_Time_2->setText("00:00");
    ui->label_Total_Time_2->setText("00:00");

    ui->listWidget_MusicList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->listWidget_MusicList->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->listWidget_MusicList->setUniformItemSizes(true);
    ui->listWidget_MusicList->setSpacing(4);
    ui->listWidget_MusicList->setAlternatingRowColors(false);
    ui->listWidget_MusicList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->listWidget_MusicList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidget_MusicList->setStyleSheet(R"(
        QListWidget#listWidget_MusicList {
            background: rgba(255,255,255,0.04);
            border: 1px solid rgba(0,238,255,0.35);
            border-radius: 10px;
            outline: 0;
        }
        QListWidget#listWidget_MusicList::item {
            height: 40px;
            padding: 6px 10px;
            color: #e8f0ff;
        }
        QListWidget#listWidget_MusicList::item:selected {
            background: rgba(0,238,255,0.18);
            border: 1px solid rgba(0,238,255,0.45);
        }
    )");

    QObject::connect(ui->listWidget_MusicList, &QListWidget::itemDoubleClicked,
                     w, [w](QListWidgetItem* it){ handleMusicSelection(w, it); });

    QObject::connect(ui->pushButton_Play_Pause_2, &QPushButton::clicked,
                     w, [w]{ handleMusicPlayPause(w); });
    QObject::connect(ui->pushButton_Stop_2, &QPushButton::clicked,
                     w, [w]{ handleMusicStop(w); });
    QObject::connect(ui->pushButton_Seek_Backward_2, &QPushButton::clicked,
                     w, [w]{ handleMusicSeekBackward(w); });
    QObject::connect(ui->pushButton_Seek_Forward_2, &QPushButton::clicked,
                     w, [w]{ handleMusicSeekForward(w); });
    QObject::connect(ui->pushButton_Volume_2, &QPushButton::clicked,
                     w, [w]{ handleMusicVolumeToggle(w); });

    QObject::connect(ui->horizontalSlider_Volume_2, &QSlider::valueChanged,
                     w, [w](int v){ handleMusicVolumeChanged(w, v); });

    QObject::connect(ui->horizontalSlider_Duration_2, &QSlider::sliderPressed,
                     w, []{ draggingPosition = true; });

    QObject::connect(ui->horizontalSlider_Duration_2, &QSlider::sliderReleased,
                     w, [w]{
        draggingPosition = false;
        musicPlayer->setPosition(w->getUi()->horizontalSlider_Duration_2->value());
    });

    QObject::connect(ui->horizontalSlider_Duration_2, &QSlider::sliderMoved,
                     w, [w](int v){
        w->getUi()->label_current_Time_2->setText(msToHMS(v));
    });

    auto setIconBtn = [](QPushButton* b, const QString& res){
        b->setText(QString());
        b->setStyleSheet("QPushButton{padding:0;border:none;background:transparent;}");
        b->setIcon(QIcon(res));
        b->setIconSize(b->size());
    };
    setIconBtn(ui->pushButton_Play_Pause_2,    ":/images/play.png");   // 초기: 재생
    setIconBtn(ui->pushButton_Stop_2,          ":/images/stop.png");
    setIconBtn(ui->pushButton_Seek_Forward_2,  ":/images/after.png");
    setIconBtn(ui->pushButton_Seek_Backward_2, ":/images/prior.png");
    setIconBtn(ui->pushButton_Volume_2,        ":/images/sound.png");

    QTimer::singleShot(0, w, [ui]{
        ui->pushButton_Play_Pause_2->setIconSize(ui->pushButton_Play_Pause_2->size());
        ui->pushButton_Stop_2->setIconSize(ui->pushButton_Stop_2->size());
        ui->pushButton_Seek_Forward_2->setIconSize(ui->pushButton_Seek_Forward_2->size());
        ui->pushButton_Seek_Backward_2->setIconSize(ui->pushButton_Seek_Backward_2->size());
        ui->pushButton_Volume_2->setIconSize(ui->pushButton_Volume_2->size());
    });

    loadMusicList(w, "./music");
}


void loadMusicList(MainWindow* w, const QString& directory)
{
    auto ui = w->getUi();
    ui->listWidget_MusicList->clear();

    QDir dir(directory);
    QStringList filters = {"*.mp3", "*.wav", "*.flac", "*.ogg", "*.m4a", "*.aac"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files|QDir::Readable, QDir::Name);

    for (const QFileInfo& fi : files) {
        auto *item = new QListWidgetItem(fi.completeBaseName(), ui->listWidget_MusicList);
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        ui->listWidget_MusicList->addItem(item);
    }
}

void handleMusicSelection(MainWindow* w, QListWidgetItem* item)
{
    Q_UNUSED(w);
    if (!item) return;
    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;

    musicPlayer->stop();
    musicPlayer->setMedia(QUrl::fromLocalFile(path));
    musicPlayer->play();

    auto btn = w->getUi()->pushButton_Play_Pause_2;
    btn->setText(QString());
    btn->setStyleSheet("QPushButton{padding:0;border:none;background:transparent;}");
    btn->setIcon(QIcon(":/images/pause.png"));
    btn->setIconSize(btn->size());
}

void handleMusicPlayPause(MainWindow* w)
{
    Q_UNUSED(w);
    auto ui  = w->getUi();
    auto btn = ui->pushButton_Play_Pause_2;

    if (musicPlayer->state() == QMediaPlayer::PlayingState) {
        musicPlayer->pause();
        btn->setIcon(QIcon(":/images/play.png"));
    } else {
        musicPlayer->play();
        btn->setIcon(QIcon(":/images/pause.png"));
    }
    btn->setIconSize(btn->size());
}

void handleMusicStop(MainWindow* w)
{
    Q_UNUSED(w);
    musicPlayer->stop();
    auto btn = w->getUi()->pushButton_Play_Pause_2;
    btn->setIcon(QIcon(":/images/play.png"));
    btn->setIconSize(btn->size());
}

void handleMusicSeekBackward(MainWindow* w)
{
    Q_UNUSED(w);
    qint64 pos = musicPlayer->position();
    musicPlayer->setPosition(qMax<qint64>(0, pos - 5000));
}

void handleMusicSeekForward(MainWindow* w)
{
    Q_UNUSED(w);
    qint64 pos = musicPlayer->position();
    qint64 dur = musicPlayer->duration();
    musicPlayer->setPosition(qMin(dur, pos + 5000));
}

void handleMusicVolumeToggle(MainWindow* w)
{
    Q_UNUSED(w);
    auto ui = w->getUi();
    isMutedMusic = !isMutedMusic;
    musicPlayer->setMuted(isMutedMusic);

    ui->pushButton_Volume_2->setIcon(QIcon(isMutedMusic?":/images/mute.png":":/images/sound.png"));
    ui->pushButton_Volume_2->setIconSize(ui->pushButton_Volume_2->size());
}

void handleMusicVolumeChanged(MainWindow* w, int value)
{
    Q_UNUSED(w);
    musicPlayer->setVolume(value);
}

void updateMusicPosition(MainWindow* w, qint64 posMs)
{
    auto ui = w->getUi();
    if (!draggingPosition) {
        ui->horizontalSlider_Duration_2->setValue(int(posMs));
        ui->label_current_Time_2->setText(msToHMS(posMs));
    }
}

void updateMusicDuration(MainWindow* w, qint64 durMs)
{
    auto ui = w->getUi();
    ui->horizontalSlider_Duration_2->setRange(0, int(durMs));
    ui->label_Total_Time_2->setText(msToHMS(durMs));
}
