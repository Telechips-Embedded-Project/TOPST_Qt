#ifndef RIPPLEWIDGET_H
#define RIPPLEWIDGET_H

#include <QWidget>
#include <QBasicTimer>
#include <QPainter>

class RippleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RippleWidget(QWidget *parent = nullptr);
    void startRipple(const QPoint &center);

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    QPoint m_center;
    int m_radius;
    qreal m_opacity;
    QBasicTimer m_timer;
};

#endif // RIPPLEWIDGET_H
