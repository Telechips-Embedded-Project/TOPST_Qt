#include "maincontainer.h"
#include <QPainter>
#include <QStackedWidget>
#include <QDebug>

MainContainer::MainContainer(QWidget *parent)
    : QWidget(parent), m_background(":/images/bg_darkwave.png")
{
}

void MainContainer::setBackground(const QString &path)
{
    QPixmap pm(path);
    if (pm.isNull()) {
        qWarning() << "[WARN] wallpaper load failed:" << path;
        return;
    }
    m_background = pm;
    update();              // 다시 그리기
}

void MainContainer::setBackground(const QPixmap &pm)
{
    if (pm.isNull()) return;
    m_background = pm;
    update();
}

void MainContainer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
//    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);                 // scale

    // Main background
    painter.drawPixmap(rect(), m_background);

    // Page background
    if (auto sw = this->findChild<QStackedWidget *>("ControlWidget")) {
        QRect swRect = sw->geometry();                                              // 로컬 기준 rect
        QPoint globalPos = sw->mapTo(this, QPoint(0, 0));                           // MainContainer 기준 좌표 보정

        QPixmap overlay(":/images/page_background.png");
        painter.setOpacity(0.7);                                                    // 반투명
        painter.drawPixmap(QRect(globalPos, swRect.size()), overlay);
//        painter.setOpacity(1.0);                                                  // 원래대로
    } else {
        qDebug() << "[WARN] stackedWidget not found in MainContainer";
    }

    QWidget::paintEvent(event);
}
