#include "ripplewidget.h"

#include <QtMath>

RippleWidget::RippleWidget(QWidget *parent)
    : QWidget(parent), m_radius(0), m_opacity(1.0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_PaintOnScreen, false);
    setAttribute(Qt::WA_AlwaysStackOnTop);

    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_UpdatesDisabled, false);

    setVisible(false);
}

void RippleWidget::startRipple(const QPoint &center)
{
    m_center = center;
    m_radius = 0;
    m_opacity = 1.0;
    setVisible(true);
    m_timer.start(16, this); // ~60fps
}

void RippleWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 그라데이션 설정
    QRadialGradient gradient(m_center, m_radius);
    QColor startColor(0, 150, 255, static_cast<int>(120 * m_opacity));  // 중심은 반투명
    QColor endColor(0, 150, 255, 0);                                    // 바깥은 완전 투명
    gradient.setColorAt(0.0, startColor);
    gradient.setColorAt(1.0, endColor);

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(m_center, m_radius, m_radius);
}

void RippleWidget::timerEvent(QTimerEvent *)
{
    m_radius += 8;
    m_opacity -= 0.04;

    // 최소한의 영역만 다시 그리기 (전체 화면 업데이트 방지)
    int r = qCeil(m_radius);
    update(QRect(m_center.x() - r, m_center.y() - r, 2*r, 2*r));

    if (m_opacity <= 0) {
        m_timer.stop();
        setVisible(false);
    } else {
//        update();
    }
}
