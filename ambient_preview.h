#ifndef AMBIENT_PREVIEW_H
#define AMBIENT_PREVIEW_H

#pragma once
#include <QWidget>
#include <QColor>
#include <QTimer>
#include <QVector>

class AmbientPreview : public QWidget
{
    Q_OBJECT
public:
    explicit AmbientPreview(QWidget* parent = nullptr);

    // level: 0=OFF, 1=LOW, 2=MEDIUM, 3=HIGH
    // color: "red","yellow","green","rainbow",""
    void applyState(int level, const QString& color);

    // 반짝임 효과 On/Off (기본 On)
    void setShimmerEnabled(bool on = true);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    int     m_level  = 0;
    QString m_color;

    // shimmer(살짝 반짝임) 관련
    bool    m_shimmer = true;
    QTimer  m_anim;           // 약 20FPS 타이머
    qreal   m_phase  = 0.0;   // 0~1 위상
    QVector<qreal> m_spotX;   // 스파클 x 위치(0~1)
    QVector<qreal> m_spotPhi; // 스파클 위상 오프셋(0~1)

    QColor currentColor() const;     // 단색(red/yellow/green) 색상
    qreal  intensityFactor() const;  // LOW/MED/HIGH → 0.40 / 0.70 / 1.00
    void   rebuildSpots(const QRectF& bar);
};

#endif // AMBIENT_PREVIEW_H
