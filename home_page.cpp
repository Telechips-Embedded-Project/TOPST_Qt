#include "home_page.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "notificationbanner.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QDateTime>
#include <QColor>

namespace {
struct Item {
    QFrame*  frame{};
    QLabel*  text{};
    QLabel*  ago{};
    QColor   accent{};
    QDateTime when{};
};

static QWidget*     s_container = nullptr;   // 리스트가 들어갈 컨테이너(스크롤 영역의 위젯)
static QVBoxLayout* s_vlay      = nullptr;   // 컨테이너의 VBox
static QTimer*      s_tick      = nullptr;   // “n분 전” 갱신 타이머
static QList<Item>  s_items;                 // 보관 중인 아이템
static int          s_capacity  = 10;        // 최대 보관 개수

// Time (N minute ago)
static QString relMinutes(const QDateTime& when) {
    if (!when.isValid()) return QStringLiteral("0분 전");
    qint64 secs = when.secsTo(QDateTime::currentDateTime());
    qint64 mins = (secs < 60) ? 0 : secs / 60;
    return QString::number(mins) + QStringLiteral("분 전");
}

static QWidget* host(MainWindow* w) {
    auto ui = w->getUi();
    return ui->widget_notificationbanner ? ui->widget_notificationbanner : ui->page_home;
}

// Make List UI
static void ensureList(MainWindow* w) {
    if (s_container) return;

    QWidget* h = host(w);
    if (!h->layout()) {
        auto v = new QVBoxLayout(h);
        v->setContentsMargins(0,0,0,0);
        v->setSpacing(0);
    }

    auto sa = new QScrollArea(h);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setWidgetResizable(true);
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    h->layout()->addWidget(sa);

    s_container = new QWidget(sa);
    sa->setWidget(s_container);

    s_vlay = new QVBoxLayout(s_container);
    s_vlay->setContentsMargins(10, 10, 10, 10);
    s_vlay->setSpacing(10);
    s_vlay->addStretch();

    // update Timer(30 sec)
    s_tick = new QTimer(s_container);
    s_tick->setInterval(30*1000);
    QObject::connect(s_tick, &QTimer::timeout, []{
        for (auto &it : s_items) {
            it.ago->setText(relMinutes(it.when));
        }
    });
    s_tick->start();
}

// Make card(banner)
static Item makeItem(const QString& msg, const QColor& accent, const QDateTime& when) {
    Item it; it.when = when; it.accent = accent;

    const int kH = 64;
    it.frame = new QFrame(s_container);
    it.frame->setFixedHeight(kH);
    it.frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    it.frame->setStyleSheet(QString(
        "QFrame {"
        " background: rgba(32,40,48,220);"
        " border-radius: 10px;"
        " border-left: 6px solid %1;"
        "}"

        "QFrame QLabel {"
        " background: transparent;"
        " border: none;"
        " border-radius: 0;"
        " padding: 0; margin: 0;"
        "}"

        "QLabel#bell { font-size: 26px; }"
    ).arg(accent.name()));

    auto row = new QHBoxLayout(it.frame);
    row->setContentsMargins(12,10,8,10);
    row->setSpacing(8);

    // Icon
    auto icon = new QLabel("🔔", it.frame);
    icon->setObjectName("bell");
    icon->setFixedWidth(28);
    icon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Text
    it.text = new QLabel(msg, it.frame);
    it.text->setWordWrap(false);
    it.text->setStyleSheet("color:white; font-size:15px;");
    it.text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // "N minute ago"
    it.ago  = new QLabel(relMinutes(when), it.frame);
    it.ago->setStyleSheet("color:#ccc; font-size:12px;");
    it.ago->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    it.ago->setMinimumWidth(60);
    it.ago->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    row->addWidget(icon);
    row->addWidget(it.text, 1);
    row->addWidget(it.ago);

    return it;
}

// Add card in List(add to Top)
static void addEntry(const QString& msg, const QColor& accent, const QDateTime& when) {
    // When capacity is exceeded, remove from the bottom (oldest)
    while (s_items.size() >= s_capacity) {
        auto old = s_items.takeLast();
        old.frame->deleteLater();
    }

    Item it = makeItem(msg, accent, when);

    // update banner TOP
    s_vlay->insertWidget(/*index*/0, it.frame);
    s_items.prepend(it);
}
} // namespace

// connect MainWindow.cpp
void initHomePage(MainWindow* w, int capacity)
{
    s_capacity = qMax(1, capacity);
    ensureList(w);
}

// connect NotificationBanner.cpp
void homeWireBanner(MainWindow* w, NotificationBanner* bn)
{
    ensureList(w);
    QObject::connect(bn, &NotificationBanner::shown, w,
        [](const QString& msg, const QColor& accent, const QDateTime& when){
            addEntry(msg, accent, when);
        });
}
