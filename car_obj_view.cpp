#include "car_obj_view.h"
#include <QFile>
#include <QTextStream>
#include <QMouseEvent>
#include <QTouchEvent>
#include <QtMath>
#include <QOpenGLContext>
#include <cstddef>

CarObjView::CarObjView(QWidget* p):QOpenGLWidget(p){
    setMinimumSize(320,220);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    // 일부 플랫폼에서 부분 업데이트가 깜빡임/충돌 유발 → 전체 갱신
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

CarObjView::~CarObjView(){
    // 컨텍스트 보유 여부 확인 후 안전 종료
    if (isValid()) {               // QOpenGLWidget 멤버 사용
        makeCurrent();
        if (vbo_.isCreated()) vbo_.destroy();
        if (ibo_.isCreated()) ibo_.destroy();
        prog_.removeAllShaders();
        doneCurrent();
    }
}

void CarObjView::setModelColor(const QColor& c){ col_=c; update(); }
void CarObjView::setBackground(const QColor& c){ bg_=c; update(); }

static int toIdx(const QString& s, int n){
    bool ok=false; int v=s.toInt(&ok); if(!ok) return -1;
    return v>0? v-1 : n+v;
}

bool CarObjView::loadObj(const QString& path)
{
    QFile f(path);
    if(!f.open(QFile::ReadOnly|QFile::Text)) {
        qWarning() << "[OBJ] open fail:" << path << f.errorString();
        return false;
    }
    QTextStream ts(&f); ts.setCodec("UTF-8");

    QVector<QVector3D> pos, nor;
    verts_.clear(); idx16_.clear();
    bbMin_={ 1e9,1e9,1e9 }; bbMax_={ -1e9,-1e9,-1e9 };

    struct Idx { int v=-1, vn=-1; };
    auto pushTri = [&](const Idx& a, const Idx& b, const Idx& c){
        V va{pos[a.v], (a.vn>=0 && a.vn<nor.size())? nor[a.vn] : QVector3D(0,0,0)};
        V vb{pos[b.v], (b.vn>=0 && b.vn<nor.size())? nor[b.vn] : QVector3D(0,0,0)};
        V vc{pos[c.v], (c.vn>=0 && c.vn<nor.size())? nor[c.vn] : QVector3D(0,0,0)};
        quint16 base = quint16(verts_.size());
        if (base > 65000) return; // 16비트 인덱스 보호
        verts_ << va << vb << vc;
        idx16_ << base << base+1 << base+2;
    };

    while(!ts.atEnd()){
        QString line = ts.readLine().trimmed();
        if(line.isEmpty() || line[0]=='#') continue;
        QTextStream ls(&line, QIODevice::ReadOnly);
        QString tag; ls >> tag;
        if(tag=="v"){
            float x,y,z; ls>>x>>y>>z; pos<<QVector3D(x,y,z);
            bbMin_.setX(qMin(bbMin_.x(),x)); bbMin_.setY(qMin(bbMin_.y(),y)); bbMin_.setZ(qMin(bbMin_.z(),z));
            bbMax_.setX(qMax(bbMax_.x(),x)); bbMax_.setY(qMax(bbMax_.y(),y)); bbMax_.setZ(qMax(bbMax_.z(),z));
        }else if(tag=="vn"){
            float x,y,z; ls>>x>>y>>z; nor<<QVector3D(x,y,z);
        }else if(tag=="f"){
            QVector<Idx> face;
            while(!ls.atEnd()){
                QString tok; ls>>tok; if(tok.isEmpty()) break;
                QStringList p = tok.split('/');
                Idx id;
                if(p.size()==1){ id.v = toIdx(p[0], pos.size()); }
                else if(p.size()>=3){ id.v = toIdx(p[0], pos.size()); id.vn = p[2].isEmpty()? -1 : toIdx(p[2], nor.size()); }
                else { id.v = toIdx(p[0], pos.size()); }
                if(id.v>=0 && id.v<pos.size()) face<<id;
            }
            for(int i=1;i+1<face.size();++i) pushTri(face[0], face[i], face[i+1]);
        }
    }
    f.close();

    computeMissingNormals();
    fitCamera();

    qDebug() << "[OBJ] stats v:" << pos.size()
          << "tris:" << (idx16_.size()/3)
          << "vertsOut:" << verts_.size();

    // ✅ 셰이더/컨텍스트 준비가 끝난 뒤에만 업로드
    if (isValid() && glReady_) {
         makeCurrent();
         uploadGL();
         doneCurrent();
    }

    update();
    return !verts_.isEmpty();
}

void CarObjView::computeMissingNormals(){
    for(int i=0;i<idx16_.size(); i+=3){
        V &a=verts_[idx16_[i]], &b=verts_[idx16_[i+1]], &c=verts_[idx16_[i+2]];
        QVector3D n = QVector3D::crossProduct(b.p-a.p, c.p-a.p);
        if(a.n.lengthSquared()==0) a.n=n;
        if(b.n.lengthSquared()==0) b.n=n;
        if(c.n.lengthSquared()==0) c.n=n;
    }
    for(auto& v: verts_) v.n.normalize();
}

void CarObjView::fitCamera(){
    center_ = 0.5f * (bbMin_ + bbMax_);           // 모델 중심
    const QVector3D size = bbMax_ - bbMin_;
    const float diag = qMax(0.001f, size.length());
    dist_ = diag * distScale_;                     // 카메라 거리
    if (!qIsFinite(dist_)) dist_ = 3.f;
}

QByteArray CarObjView::vsSrc() const {
    QByteArray v;

    bool es = (this->context() && this->context()->isOpenGLES());
    v += es ? "#version 100\n" : "#version 120\n";
    v +=
    "attribute vec3 aPos;\n"
    "attribute vec3 aNor;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat3 uNrm;\n"
    "uniform vec3 uLightDir;\n"
    "varying float vLit;\n"
    "void main(){ vec3 n=normalize(uNrm*aNor); vLit=max(dot(n,normalize(uLightDir)),0.25); gl_Position=uMVP*vec4(aPos,1.0);} \n";
    return v;
}
QByteArray CarObjView::fsSrc() const {
    QByteArray f;
    bool es = (this->context() && this->context()->isOpenGLES());
    f += es ? "#version 100\nprecision mediump float;\n" : "#version 120\n";
    f += "uniform vec3 uColor; varying float vLit; void main(){ gl_FragColor=vec4(uColor*vLit,1.0); }";
    return f;
}

void CarObjView::initializeGL(){
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    prog_.addShaderFromSourceCode(QOpenGLShader::Vertex,   vsSrc());
    prog_.addShaderFromSourceCode(QOpenGLShader::Fragment, fsSrc());
    prog_.bindAttributeLocation("aPos", 0);
    prog_.bindAttributeLocation("aNor", 1);
    prog_.link();

    glReady_ = true;

    // (선택) 드라이버 정보 로깅 — 이 줄은 컨텍스트가 있으니 안전
    // qDebug() << "GL_VENDOR=" << (const char*)glGetString(GL_VENDOR)
    //          << "GL_RENDERER=" << (const char*)glGetString(GL_RENDERER)
    //          << "GL_VERSION="  << (const char*)glGetString(GL_VERSION);

    // ❌ 여기서 uploadGL() 호출하지 마세요 (초기화 타이밍 레이스 방지)
}

void CarObjView::resizeGL(int w,int h){ Q_UNUSED(w); Q_UNUSED(h); }

void CarObjView::uploadGL(){
    if (!glReady_) return;

    if(!vbo_.isCreated()) vbo_.create();
    if(!ibo_.isCreated()) ibo_.create();

    vbo_.bind(); vbo_.allocate(verts_.constData(), verts_.size()*sizeof(V));
    ibo_.bind(); ibo_.allocate(idx16_.constData(), idx16_.size()*sizeof(quint16));

    // VAO 없이, 한 번만 속성 포인터 설정(일부 드라이버는 매 프레임 설정이 안전)
    prog_.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), reinterpret_cast<void*>(offsetof(V,p)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), reinterpret_cast<void*>(offsetof(V,n)));
    prog_.release();

    vbo_.release(); ibo_.release();
}

void CarObjView::paintGL(){
    glClearColor(bg_.redF(), bg_.greenF(), bg_.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    if (verts_.isEmpty()) return;

    // 🔐 최초 프레임 등에서 VBO/IBO가 아직 없으면 지금 생성
    if (!vbo_.isCreated() || !ibo_.isCreated()) {
     uploadGL();
     if (!vbo_.isCreated() || !ibo_.isCreated())
         return; // 아직 준비 안 됐으면 그리지 않음(안전탈출)
    }

    QMatrix4x4 P; P.perspective(45.f, float(width())/qMax(1.0f,float(height())), 0.01f, 5000.f);
    QMatrix4x4 V; V.lookAt(QVector3D(0,0,dist_), QVector3D(0,0,0), QVector3D(0,1,0));
    QMatrix4x4 R; R.rotate(yaw_,0,1,0); R.rotate(pitch_,1,0,0);
    QMatrix4x4 M = R;

    prog_.bind();
    prog_.setUniformValue("uMVP", P*V*M);
    prog_.setUniformValue("uNrm", M.normalMatrix());
    prog_.setUniformValue("uLightDir", QVector3D(-0.6f,0.8f,0.4f));
    prog_.setUniformValue("uColor", QVector3D(col_.redF(), col_.greenF(), col_.blueF()));

    vbo_.bind(); ibo_.bind();
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CarObjView::V),
                       reinterpret_cast<void*>(offsetof(CarObjView::V, p)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CarObjView::V),
                       reinterpret_cast<void*>(offsetof(CarObjView::V, n)));

    glDrawElements(GL_TRIANGLES, idx16_.size(), GL_UNSIGNED_SHORT, nullptr);

    ibo_.release(); vbo_.release();
    prog_.release();
}

void CarObjView::mousePressEvent(QMouseEvent* e){ lastPos_=e->pos(); }
void CarObjView::mouseMoveEvent(QMouseEvent* e){
    QPoint d=e->pos()-lastPos_; lastPos_=e->pos();
    yaw_ += d.x()*0.4f; pitch_ += d.y()*0.4f; pitch_=qBound(-89.f,pitch_,89.f);
    update();
}
void CarObjView::wheelEvent(QWheelEvent* e){
#if QT_VERSION >= QT_VERSION_CHECK(5,14,0)
    dist_ *= (e->angleDelta().y()>0? 0.9f : 1.1f);
#else
    dist_ *= (e->delta()>0? 0.9f : 1.1f);
#endif
    dist_ = qBound(0.2f, dist_, 10000.f);
    update();
}
bool CarObjView::event(QEvent* ev){
    if(ev->type()==QEvent::TouchBegin || ev->type()==QEvent::TouchUpdate){
        auto* te=static_cast<QTouchEvent*>(ev);
        if(!te->touchPoints().isEmpty()){
            static QPointF last = te->touchPoints().first().pos();
            QPointF p = te->touchPoints().first().pos();
            QPointF d = p-last; last=p;
            yaw_ += d.x()*0.3f; pitch_ += d.y()*0.3f; pitch_=qBound(-89.f,pitch_,89.f);
            update();
        }
        return true;
    }
    return QOpenGLWidget::event(ev);
}
