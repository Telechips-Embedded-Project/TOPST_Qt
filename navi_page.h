#ifndef NAVI_PAGE_H
#define NAVI_PAGE_H

#include <QListWidget>  // 필요시
#include <QPushButton>

class MainWindow;

// 초기화 및 신호 연결 함수

void initNaviConnections(MainWindow* w);

// 내비게이션 관련 기능 처리핸들러 예시
void handleNaviListSelection(MainWindow* w, QListWidgetItem* item);
void handleNaviSomeButton(MainWindow* w, QPushButton* sender);

#endif // NAVI_PAGE_H
