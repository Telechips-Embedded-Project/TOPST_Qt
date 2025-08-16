#ifndef MUSIC_PAGE_H
#define MUSIC_PAGE_H

#include <QString>

class MainWindow;
class QListWidgetItem;

void initMusicConnections(MainWindow* w);
void loadMusicList(MainWindow* w, const QString& directory);

void handleMusicSelection(MainWindow* w, QListWidgetItem* item);
void handleMusicPlayPause(MainWindow* w);
void handleMusicStop(MainWindow* w);
void handleMusicVolumeToggle(MainWindow* w);
void handleMusicVolumeChanged(MainWindow* w, int value);
void handleMusicSeekBackward(MainWindow* w);
void handleMusicSeekForward(MainWindow* w);

void updateMusicPosition(MainWindow* w, qint64 posMs);
void updateMusicDuration(MainWindow* w, qint64 durMs);

// cover helper
void applyMusicCover(MainWindow* w, const QString& coverPath);
void resetMusicCover(MainWindow* w);

#endif // MUSIC_PAGE_H
