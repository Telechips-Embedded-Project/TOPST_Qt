#include "ambient_preview.h"
#include <QPainter>
#include <QLinearGradient>
#include <QSizePolicy>
//#include <QRandomGenerator>
#include "account/random_compat.h"
#include <QResizeEvent>
#include <QtMath>

static constexpr qreal TAU = 6.28318530717958647692;

static QColor mixWhite(const QColor& c, qreal w) {
    w = qBound<qreal>(0.0, w, 1.0);
    int r = c.red(), g = c.green(), b = c.blue(), a = c.alpha();
    r = int(r + (255 - r) * w);
    g = int(g + (255 - g) * w);
    b = int(b + (255 - b) * w);
    return QColor(r, g, b, a);
}

AmbientPreview::AmbientPreview(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(140);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);

    // 20 FPS
    m_anim.setInterval(48);
    connect(&m_anim, &QTimer::timeout, this, [this]{
        // 부드러운 호흡: 0~1 위상을 조금씩 증가
        m_phase += 0.02;
        if (m_phase > 1.0) m_phase -= 1.0;
        update();
    });
}

void AmbientPreview::setShimmerEnabled(bool on)
{
    m_shimmer = on;
    if (!m_shimmer) m_anim.stop();
    else if (m_level > 0 && !m_anim.isActive()) m_anim.start();
    update();
}

void AmbientPreview::applyState(int level, const QString& color)
{
    m_level = qBound(0, level, 3);
    m_color = color;

    // 레벨>0이고 shimmer가 켜져있으면 타이머 가동
    if (m_shimmer && m_level > 0) {
        if (!m_anim.isActive()) m_anim.start();
    } else {
        if (m_anim.isActive()) m_anim.stop();
    }

    update();
}

QColor AmbientPreview::currentColor() const
{
    const QString c = m_color.toLower();
    if (c == "red")     return QColor(255, 70, 70);
    if (c == "yellow")  return QColor(255, 210, 80);
    if (c == "green")   return QColor(80, 230, 160);

    return QColor(0, 220, 220);
}

qreal AmbientPreview::intensityFactor() const
{
    switch (m_level) {
        case 1: return 0.40;   // LOW
        case 2: return 0.70;   // MEDIUM
        case 3: return 1.00;   // HIGH
        default: return 0.00;  // OFF
    }
}

void AmbientPreview::rebuildSpots(const QRectF& bar)
{
    int N = qBound(6, int(bar.width() / 90.0), 12);
    m_spotX.resize(N);
    m_spotPhi.resize(N);

    /*
    QRandomGenerator rng(12345); // 결정적 분포(리사이즈 시 흔들림 최소화)
    for (int i = 0; i < N; ++i) {

        m_spotX[i]   = 0.08 + 0.84 * rng.generateDouble();
        m_spotPhi[i] = rng.generateDouble();
    }
    */
    for (int i = 0; i < N; ++i) {
        m_spotX[i]   = 0.08 + 0.84 * rand01(); // 0..1
        m_spotPhi[i] = rand01();               // 0..1
    }
}

void AmbientPreview::resizeEvent(QResizeEvent* e)
{
    // 바 영역 기준으로 스팟 재배치
    QRectF panel = rect().adjusted(10, 8, -12, -8);
    const qreal barH = 36.0;
    QRectF bar(panel.left() + 22,
               panel.center().y() - barH / 2,
               panel.width() - 44,
               barH);
    rebuildSpots(bar);
    QWidget::resizeEvent(e);
}

void AmbientPreview::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 패널(라운드 카드)
    QRectF panel = rect().adjusted(10, 8, -12, -8);
    /*
    const qreal pr = 22.0;

    QLinearGradient bg(panel.topLeft(), panel.bottomLeft());
    bg.setColorAt(0.0, QColor(10, 16, 24, 200));
    bg.setColorAt(1.0, QColor(6, 10, 16, 230));

    p.setPen(QPen(QColor(20, 34, 48, 220), 2));
    p.setBrush(bg);
    p.drawRoundedRect(panel, pr, pr);
    */

    // LED 스트립(고정 두께 중앙 배치)
    const qreal barH = 36.0;
    QRectF bar(panel.left() + 22,
               panel.center().y() - barH / 2,
               panel.width() - 44,
               barH);

    // OFF: 희미한 가이드 바
    if (m_level <= 0) {
        p.setPen(Qt::NoPen);
        QLinearGradient g(bar.left(), bar.top(), bar.right(), bar.top());
        g.setColorAt(0.0, QColor(0, 128, 255, 60));
        g.setColorAt(1.0, QColor(0, 255, 200, 30));
        p.setBrush(g);
        p.drawRoundedRect(bar, barH/2, barH/2);
        return;
    }

    const qreal k = intensityFactor();
    const bool isRainbow = (m_color.compare("rainbow", Qt::CaseInsensitive) == 0);

    // 부드러운 '호흡' 계수 (전체 밝기/알파에 미세 반영)
    const qreal breath = 1.0 + 0.05 * qSin(TAU * m_phase); // ±5%

    // Glow(다중 스트로크) — rainbow일 땐 약간 더 강하게 + 호흡 반영
    for (int i = 6; i >= 1; --i) {
        qreal t = i / 6.0;
        qreal w = barH + t * 16.0;
        qreal a = 28.0 * k * t * (isRainbow ? 1.20 : 1.0) * breath; // breath 반영
        QColor gc = isRainbow
                    ? QColor::fromHsvF(0.15 + 0.7 * t, 1.0, 1.0)
                    : currentColor();
        gc.setAlphaF(qBound(0.0, a / 255.0, 1.0));

        p.setPen(QPen(gc, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(QPointF(bar.left(),  bar.center().y()),
                   QPointF(bar.right(), bar.center().y()));
    }

    // LED 바 본체
    p.setPen(Qt::NoPen);
    if (isRainbow) {
        // 세기에 따라 하양 섞기 비율 ↑ → 바 본체가 확실히 밝아짐
        const qreal wmix = (0.10 + 0.35 * k) * breath;  // LOW≈0.24, MED≈0.345, HIGH≈0.45
        QLinearGradient g(bar.left(), bar.top(), bar.right(), bar.top());
        g.setSpread(QGradient::PadSpread);

        auto add = [&](qreal pos, const QColor& c){ g.setColorAt(pos, mixWhite(c, wmix)); };
        add(0.00, QColor(255,  60,  60));
        add(0.17, QColor(255, 160,  60));
        add(0.33, QColor(255, 230,  60));
        add(0.50, QColor( 60, 230, 120));
        add(0.67, QColor( 60, 160, 255));
        add(0.83, QColor(180,  60, 255));
        add(1.00, QColor(255,  60,  60));

        p.setBrush(g);
        p.drawRoundedRect(bar, barH/2, barH/2);
    } else {
        QColor c = currentColor();
        c = c.lighter(int((100 + k * 60) * breath));   // 100~160%에 breath(+/-5%)
        QLinearGradient g(bar.left(), bar.top(), bar.right(), bar.top());
        g.setColorAt(0.0, c.lighter(120));
        g.setColorAt(0.5, c);
        g.setColorAt(1.0, c.lighter(120));
        p.setBrush(g);
        p.drawRoundedRect(bar, barH/2, barH/2);
    }

    // 스파클(작은 반짝임) — 제자리에서 서서히 나타났다 사라짐 (이동 없음)
    if (m_shimmer && !m_spotX.isEmpty()) {
        for (int i = 0; i < m_spotX.size(); ++i) {
            // 각 스팟의 위상: m_phase에 고유 오프셋
            qreal phi = m_phase * 1.3 + m_spotPhi[i];
            qreal pulse = 0.5 + 0.5 * qSin(TAU * phi); // 0~1
            qreal alpha = 0.18 * k * pulse;            // 최대 18% * 세기
            if (alpha < 0.02) continue;

            qreal x = bar.left() + m_spotX[i] * bar.width();
            qreal R = barH * 0.55; // 스팟 반경

            QRadialGradient rg(QPointF(x, bar.center().y()), R);
            QColor cw = QColor(255, 255, 255, int(255 * alpha));
            QColor cz = QColor(255, 255, 255, 0);
            rg.setColorAt(0.0, cw);
            rg.setColorAt(1.0, cz);

            p.setBrush(rg);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(x, bar.center().y()), R, R * 0.85);
        }
    }
}
