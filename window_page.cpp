#include "window_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMovie>
#include <QDebug>

static QMovie* windowOpenMovie = nullptr;
static QMovie* windowCloseMovie = nullptr;

void initWindowConnections(MainWindow* w)
{
    auto ui = w->getUi();

    ui->pushButton_window_open->setText("");
    ui->pushButton_window_close->setText("");

    ui->pushButton_window_open->setStyleSheet("border-image: url(:/images/WINDOW_OPEN.png);");
    ui->pushButton_window_close->setStyleSheet("border-image: url(:/images/WINDOW_CLOSE.png);");

    if (!windowOpenMovie) {
        windowOpenMovie = new QMovie(":/gif/window_open_gif.gif");
    }

    if (!windowCloseMovie) {
        windowCloseMovie = new QMovie(":/gif/window_close_gif.gif");
    }

    // 초기 표시용 - open GIF의 첫 프레임
    ui->label_window_gif->setMovie(windowOpenMovie);
    windowOpenMovie->start();
    windowOpenMovie->stop();
    windowOpenMovie->jumpToFrame(0);

    // Resezied GIF size
    QObject::connect(ui->ControlWidget, &QStackedWidget::currentChanged, [=](int index) {
        if (ui->ControlWidget->widget(index) == ui->page_window) {
            if (windowOpenMovie) {
                windowOpenMovie->setScaledSize(ui->label_window_gif->size());
                ui->label_window_gif->setMovie(windowOpenMovie);
                windowOpenMovie->stop();
                windowOpenMovie->jumpToFrame(0);
            }
            if (windowCloseMovie) {
                windowCloseMovie->setScaledSize(ui->label_window_gif->size());
                windowCloseMovie->stop();
            }
        }
    });

    QObject::connect(ui->pushButton_window_open, &QPushButton::pressed, [=]() {
        onWindowOpenPressed(w);
    });
    QObject::connect(ui->pushButton_window_open, &QPushButton::released, [=]() {
        onWindowOpenReleased(w);
    });

    QObject::connect(ui->pushButton_window_close, &QPushButton::pressed, [=]() {
        onWindowClosePressed(w);
    });
    QObject::connect(ui->pushButton_window_close, &QPushButton::released, [=]() {
        onWindowCloseReleased(w);
    });
}

/*
// connect
void initWindowConnections(MainWindow* w)
{
    auto ui = w->getUi();

    // 버튼 텍스트 제거
    ui->pushButton_window_open->setText("");
    ui->pushButton_window_close->setText("");

    // 버튼 이미지 설정
    ui->pushButton_window_open->setStyleSheet("border-image: url(:/images/WINDOW_OPEN.png);");
    ui->pushButton_window_close->setStyleSheet("border-image: url(:/images/WINDOW_CLOSE.png);");

    // 열기 GIF 초기화
    if (!windowOpenMovie) {
        windowOpenMovie = new QMovie(":/gif/window_open_gif.gif");
        windowOpenMovie->setScaledSize(ui->label_window_gif->size());
        windowOpenMovie->stop();                // 정지 상태
        windowOpenMovie->jumpToFrame(0);        // 첫 프레임 보여주기
    }

    // 닫기 GIF 초기화
    if (!windowCloseMovie) {
        windowCloseMovie = new QMovie(":/gif/window_close_gif.gif");
        windowCloseMovie->setScaledSize(ui->label_window_gif->size());
        windowCloseMovie->stop();
        windowCloseMovie->jumpToFrame(0);
    }

    // 초기에는 open GIF의 첫 프레임을 보여줌
    ui->label_window_gif->setMovie(windowOpenMovie);
    ui->label_window_gif->setScaledContents(true);

    // OPEN 버튼 동작 연결
    QObject::connect(ui->pushButton_window_open, &QPushButton::pressed, [=]() {
        onWindowOpenPressed(w);
    });
    QObject::connect(ui->pushButton_window_open, &QPushButton::released, [=]() {
        onWindowOpenReleased(w);
    });

    // CLOSE 버튼 동작 연결
    QObject::connect(ui->pushButton_window_close, &QPushButton::pressed, [=]() {
        onWindowClosePressed(w);
    });
    QObject::connect(ui->pushButton_window_close, &QPushButton::released, [=]() {
        onWindowCloseReleased(w);
    });
}
*/

// OPEN 누름
void onWindowOpenPressed(MainWindow* w)
{
    auto ui = w->getUi();
    auto btn = ui->pushButton_window_open;
    QSize sz = btn->size();
    QPoint pos = btn->pos();

    btn->setText("");
    btn->resize(sz * 0.9);
    btn->move(pos + QPoint(sz.width() * 0.05, sz.height() * 0.05));

    // GIF 재생
    if (windowOpenMovie != nullptr) {
        ui->label_window_gif->setMovie(windowOpenMovie);
        windowOpenMovie->start();
    }

    QString json = R"({"device":"window","command":"open"})";
    w->sendJsonToFifo(json);
}

// OPEN 뗌
void onWindowOpenReleased(MainWindow* w)
{
    auto ui = w->getUi();
    auto btn = ui->pushButton_window_open;
    QSize sz = btn->size() / 0.9;
    QPoint pos = btn->pos() - QPoint(sz.width() * 0.05, sz.height() * 0.05);

    btn->setText("");
    btn->resize(sz);
    btn->move(pos);

    ui->pushButton_window_open->setStyleSheet("border-image: url(:/images/WINDOW_OPEN.png);");
    ui->pushButton_window_close->setStyleSheet("border-image: url(:/images/WINDOW_CLOSE.png);");

    // GIF 멈춤
    if (windowOpenMovie != nullptr) {
        windowOpenMovie->stop();
    }

    // FIFO
    QString json = R"({"device":"window","command":"stop"})";
    w->sendJsonToFifo(json);
}

// CLOSE 누름
void onWindowClosePressed(MainWindow* w)
{
    auto ui = w->getUi();
    auto btn = ui->pushButton_window_close;
    QSize sz = btn->size();
    QPoint pos = btn->pos();

    btn->setText("");
    btn->resize(sz * 0.9);
    btn->move(pos + QPoint(sz.width() * 0.05, sz.height() * 0.05));

    // GIF 재생
    if (windowCloseMovie != nullptr) {
        ui->label_window_gif->setMovie(windowCloseMovie);
        windowCloseMovie->start();
    }

    // FIFO
    QString json = R"({"device":"window","command":"close"})";
    w->sendJsonToFifo(json);
}

// CLOSE 뗌
void onWindowCloseReleased(MainWindow* w)
{
    auto ui = w->getUi();
    auto btn = ui->pushButton_window_close;
    QSize sz = btn->size() / 0.9;
    QPoint pos = btn->pos() - QPoint(sz.width() * 0.05, sz.height() * 0.05);

    btn->setText("");
    btn->resize(sz);
    btn->move(pos);

    ui->pushButton_window_open->setStyleSheet("border-image: url(:/images/WINDOW_OPEN.png);");
    ui->pushButton_window_close->setStyleSheet("border-image: url(:/images/WINDOW_CLOSE.png);");

    // GIF 멈춤
    if (windowCloseMovie != nullptr) {
        windowCloseMovie->stop();
    }

    // FIFO
    QString json = R"({"device":"window","command":"stop"})";
    w->sendJsonToFifo(json);
}

