#include "mainwindow.h"
#include "account/accountselectpage.h"
//#include "account/accountsettings.h"
//#include "account/accountmanager.h"

#include <QApplication>
#include <QTimer>
#include <QDateTime>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    MainWindow w;
//    w.show();

    // 계정 선택 화면 생성 및 띄움
    AccountSelectPage *selectPage = new AccountSelectPage();
    selectPage->show();

    QObject::connect(selectPage, &AccountSelectPage::accountSelected,
                     [&](const QString &accountName) {
        qDebug() << "선택된 계정:" << accountName;

        // mainwindow
        MainWindow *mainWin = new MainWindow();
        mainWin->show();

        selectPage->close();
    });

    return a.exec();
}
