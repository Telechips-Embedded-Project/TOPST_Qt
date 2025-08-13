#ifndef HOME_PAGE_H
#define HOME_PAGE_H

class MainWindow;
class NotificationBanner; // 배너 신호 연결용

void initHomePage(MainWindow* w, int capacity = 10);         // Max: 10
void homeWireBanner(MainWindow* w, NotificationBanner* bn);  // banner shown → add list

#endif // HOME_PAGE_H
