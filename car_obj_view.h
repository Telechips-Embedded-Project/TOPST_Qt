#ifndef CAR_OBJ_VIEW_H
#define CAR_OBJ_VIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>

class CarObjView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit CarObjView(QWidget* parent=nullptr);
    ~CarObjView() override;

    bool loadObj(const QString& path);          // v/vn/f 지원(uv/mtl 무시)
    void setModelColor(const QColor& c);
    void setBackground(const QColor& c);

    void setInitialView(float yawDeg, float pitchDeg, float distanceScale = 2.0f) {
        initYaw_ = yawDeg; initPitch_ = pitchDeg; distScale_ = distanceScale;
        yaw_ = initYaw_; pitch_ = initPitch_;
        fitCamera(); update();
    }
    void resetView() { yaw_ = initYaw_; pitch_ = initPitch_; fitCamera(); update(); }

protected:
    void initializeGL() override;
    void resizeGL(int w,int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    bool event(QEvent*) override;

private:
    QVector3D center_;           // 모델 중심
    float initYaw_   = -35.f;    // 원하는 시작 각도
    float initPitch_ = -15.f;
    float distScale_ = 2.0f;     // 화면 채우는 정도(모델 대각선 * 이 값)

    struct V { QVector3D p; QVector3D n; };
    QVector<V> verts_;
    // ES2 호환을 위해 16bit 인덱스 사용(버텍스가 65,535개 이하인 모델 권장)
    QVector<quint16> idx16_;

    QVector3D bbMin_{ 1e9, 1e9, 1e9 }, bbMax_{ -1e9,-1e9,-1e9 };

    QOpenGLBuffer vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer ibo_{QOpenGLBuffer::IndexBuffer};
    QOpenGLShaderProgram prog_;

    QColor bg_{10,16,24}, col_{210,220,230};
    float yaw_=20.f, pitch_=10.f, dist_=3.f;
    QPoint lastPos_;

    bool glReady_=false;

    void uploadGL();
    void computeMissingNormals();
    void fitCamera();

    QByteArray vsSrc() const;
    QByteArray fsSrc() const;
};

#endif
