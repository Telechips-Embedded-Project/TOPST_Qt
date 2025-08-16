#include "notificationbanner.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QEvent>
#include <QDateTime>

NotificationBanner::NotificationBanner(QWidget *parent)
    : QFrame(parent),
      m_icon(new QLabel(this)),
      m_text(new QLabel(this)),
      m_close(new QPushButton("✕", this)),
      m_opacity(new QGraphicsOpacityEffect(this)),
      m_slide(new QPropertyAnimation(this, "pos", this)),
      m_fade(new QPropertyAnimation(m_opacity, "opacity", this)),
      m_autoHide(new QTimer(this)),
      m_accent("#00aaff")
{
    setObjectName("NotificationBanner");
    setAttribute(Qt::WA_StyledBackground, true);
    setGraphicsEffect(m_opacity);
    setFixedHeight(64);

    // Notification text
    m_icon->setText("🔔");
    m_icon->setFixedWidth(32);
    m_text->setWordWrap(true);

    QHBoxLayout *lay = new QHBoxLayout(this);
    lay->setContentsMargins(12, 10, 8, 10);
    lay->setSpacing(8);
    lay->addWidget(m_icon);
    lay->addWidget(m_text, 1);
    lay->addWidget(m_close);

    applyStyle();

    m_slide->setDuration(280);
    m_slide->setEasingCurve(QEasingCurve::OutCubic);

    m_fade->setDuration(200);

    // Auto hide
    m_autoHide->setSingleShot(true);
    connect(m_autoHide, SIGNAL(timeout()), this, SLOT(hideBanner()));
    connect(m_close, SIGNAL(clicked()), this, SLOT(hideBanner()));

    if (parent) parent->installEventFilter(this);
}

void NotificationBanner::applyStyle() {
    setStyleSheet(QString(
          "#NotificationBanner {"
          " background: rgba(32, 40, 48, 220);"
          " border-radius: 10px;"
          " border-left: 6px solid %1;"
          "}"
          "QLabel { color: white; font-size: 15px; }"
          "QLabel#bell { font-size: 24px; }"
          "QPushButton { border:0; color:#ddd; font-weight:bold; }"
          "QPushButton:hover { color:white; }"
    ).arg(m_accent.name()));
}

void NotificationBanner::setMessage(const QString &msg) {
    m_text->setText(msg);
}

void NotificationBanner::setAccentColor(const QColor &c) {
    m_accent = c;
    applyStyle();
}

void NotificationBanner::positionAtTop() {
    QWidget *p = parentWidget();
    if (!p) return;
    const int margin = 10;
    int w = qMin(p->width() - margin*2, 600);
    setFixedWidth(w);
    int x = (p->width() - w) / 2;
    move(x, y());
}

bool NotificationBanner::eventFilter(QObject *obj, QEvent *ev) {
    if (obj == parent() && (ev->type() == QEvent::Resize || ev->type() == QEvent::Move)) {
        positionAtTop();
    }
    return QFrame::eventFilter(obj, ev);
}

void NotificationBanner::showBanner(int msec) {
    QWidget *p = parentWidget();
    if (!p) return;

    m_autoHide->stop();
    disconnect(m_slide, SIGNAL(finished()), 0, 0);

    positionAtTop();
    const int margin = 10;

    int x0 = this->x();
    int startY = -height();
    int endY = margin;

    m_opacity->setOpacity(1.0);
    move(x0, startY);
    show();
    raise();

    m_slide->stop(); m_fade->stop();
    m_slide->setDuration(280);
    m_slide->setEasingCurve(QEasingCurve::OutCubic);
    m_slide->setStartValue(QPoint(x0, startY));
    m_slide->setEndValue(QPoint(x0, endY));
    m_slide->start();

    m_autoHide->start(qMax(100, msec));

    emit shown(m_text->text(), m_accent, QDateTime::currentDateTime());
}

void NotificationBanner::hideBanner() {
    const int x0 = x();
    const int endY = -height();

    m_slide->stop(); m_fade->stop();

    m_slide->setDuration(220);
    m_slide->setEasingCurve(QEasingCurve::InCubic);
    m_slide->setStartValue(QPoint(x0, y()));
    m_slide->setEndValue(QPoint(x0, endY));

    m_fade->setStartValue(1.0);
    m_fade->setEndValue(0.0);

    // finish 연결 중복 방지
    disconnect(m_slide, SIGNAL(finished()), 0, 0);
    connect(m_slide, &QPropertyAnimation::finished, this, [this]{
        hide();
        m_opacity->setOpacity(1.0);
    });

    m_slide->start();
    m_fade->start();
}
