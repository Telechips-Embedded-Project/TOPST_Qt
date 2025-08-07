#ifndef VIDEO_PAGE_H
#define VIDEO_PAGE_H

#include <QListWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QDir>

class MainWindow;

void initVideoConnections(MainWindow* w);
void loadVideoList(MainWindow* w, const QString& directory);
void handleVideoSelection(MainWindow* w, QListWidgetItem* item);
void handlePlayPause(MainWindow* w);
void handleStop(MainWindow* w);
void handleVolumeToggle(MainWindow* w);
void handleVolumeChanged(MainWindow* w, int value);
void handleSeekBackward(MainWindow* w);
void handleSeekForward(MainWindow* w);
void updateVideoPosition(MainWindow* w, qint64 duration);
void updateVideoDuration(MainWindow* w, qint64 duration);

#endif // VIDEO_PAGE_H
