#include "mainwindow.h"
#include "account/accountselectpage.h"
#include "db/db.h"

#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Db::initMaster();

    int id = QFontDatabase::addApplicationFont(":/fonts/NotoEmoji-Regular.ttf");
    if (id < 0) {
        qWarning() << "emoji font load failed";
    }

//    MainWindow w;
//    w.show();

    // 계정 선택 화면 생성 및 띄움
    AccountSelectPage *selectPage = new AccountSelectPage();
    selectPage->show();

    /*
    QObject::connect(selectPage, &AccountSelectPage::accountSelected,
                     [&](const QString &accountName) {
        qDebug() << "선택된 계정:" << accountName;
    */
    QObject::connect(selectPage, &AccountSelectPage::accountSelected,
                      [&](int accountId, const QString &accountName) {
         qDebug() << "선택된 계정:" << accountId << accountName;

        // mainwindow
        MainWindow *mainWin = new MainWindow();
        mainWin->setCurrentUser(accountId);
        mainWin->show();

        selectPage->close();
    });

    return a.exec();
}
