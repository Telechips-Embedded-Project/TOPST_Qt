#ifndef NOTIFICATIONBANNER_H
#define NOTIFICATIONBANNER_H

#pragma once
#include <QFrame>
class QLabel; class QPushButton;
class QPropertyAnimation; class QTimer; class QGraphicsOpacityEffect;

class NotificationBanner : public QFrame {
    Q_OBJECT
public:
    explicit NotificationBanner(QWidget *parent = nullptr);
    void setMessage(const QString &msg);
    void setAccentColor(const QColor &c);
    void showBanner(int msec = 2500);  // 자동 숨김 시간(ms)

signals:
    void shown(const QString& msg, const QColor& accent, const QDateTime& when);

public slots:
    void hideBanner();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void applyStyle();
    void positionAtTop();

    QLabel *m_icon;
    QLabel *m_text;
    QPushButton *m_close;
    QGraphicsOpacityEffect *m_opacity;
    QPropertyAnimation *m_slide;
    QPropertyAnimation *m_fade;
    QTimer *m_autoHide;
    QColor m_accent;
};

#endif // NOTIFICATIONBANNER_H
