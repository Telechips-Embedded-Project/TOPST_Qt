#include "accountselectpage.h"
#include "ui_accountselectpage.h"
#include "random_compat.h"

#include "db/db.h"
#include <QSet>

#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QtMath>
#include <QDebug>
#include <QIcon>
#include <QPixmap>

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QGridLayout>
#include <QSpacerItem>
#include <QStyle>
#include <QScreen>

AccountSelectPage::AccountSelectPage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AccountSelectPage)
{
    ui->setupUi(this);

    // NEW btn
    ui->pushButton_addNew->setStyleSheet(R"(
        QPushButton {
            background-color: #124;
            color: white;
            font-size: 16pt;
            border-radius: 15px;
            padding: 10px;
        }
        QPushButton:hover { background-color: #666; }
    )");
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    // new
    Db::initMaster();
    refreshAccountButtons();
}

AccountSelectPage::~AccountSelectPage()
{
    delete ui;
}

void AccountSelectPage::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    QPixmap bg(":/images/account_background.png");
    if (!bg.isNull())
        painter.drawPixmap(rect(), bg);
}

QPixmap AccountSelectPage::makeRoundedCover(const QString& path, const QSize& size, int radius)
{
    QPixmap src(path);
    QPixmap scaled;

    if (src.isNull()) {
        scaled = QPixmap(size);
        scaled.fill(QColor("#12243a"));
    } else {
        scaled = src.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    }

    QPixmap out(size);
    out.fill(Qt::transparent);

    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath clip;
    clip.addRoundedRect(QRectF(0,0,size.width(), size.height()), radius, radius);
    p.setClipPath(clip);
    p.drawPixmap(0, 0, scaled);

    return out;
}

QString AccountSelectPage::pickRandomImagePath() const
{
    static const QStringList kCandidates = {
        ":/account/images/Angry.png",
        ":/account/images/Bob.png",
        ":/account/images/Graph.png",
        ":/account/images/killer.png",
        ":/account/images/Rabbit.png",
        ":/account/images/Robot.png",
        ":/account/images/Sheep.png",
        ":/account/images/who.png",
    };
    if (kCandidates.isEmpty()) return QString();
    int idx = int(rand01() * kCandidates.size());
    if (idx >= kCandidates.size()) idx = kCandidates.size() - 1;
    return kCandidates.at(idx);
}

QWidget* AccountSelectPage::makeAccountButtonWidget(const QString& name, int id,
                                                    const QSize& btnSize, const QString& imagePath)
{
    auto *accountBtn = new QPushButton;
    accountBtn->setObjectName(QString("account_%1").arg(id));
    accountBtn->setFixedSize(btnSize);

    QPixmap rounded = makeRoundedCover(imagePath, btnSize, 15);

    accountBtn->setStyleSheet(R"(
        QPushButton { padding: 0px; border: none; border-radius: 15px; background-color: #12243a; }
        QPushButton:hover   { background-color: #1b3553; }
        QPushButton:pressed { background-color: #0f1b2c; }
    )");
    accountBtn->setIcon(QIcon(rounded));
    accountBtn->setIconSize(btnSize);

    // ---------- 길게 눌러 삭제모드 진입 ----------
    auto holdTimer = new QTimer(accountBtn);
    holdTimer->setSingleShot(true);
    holdTimer->setInterval(1200); // 1.2s long-press

    connect(accountBtn, &QPushButton::pressed, this, [=](){
        holdTimer->start();
    });
    connect(accountBtn, &QPushButton::released, this, [=](){
        // 길게 누르지 않은 '짧은 클릭'만 선택으로 처리
        if (holdTimer->isActive()) {
            holdTimer->stop();
            if (!m_deleteMode)
                emit accountSelected(id, name);
        }
        // 길게 눌러 timeout이 발생한 경우엔 enterDeleteMode()에서 처리됨
    });
    connect(holdTimer, &QTimer::timeout, this, [=](){
        enterDeleteMode();          // 내부에서 refreshAccountButtons() 호출한다고 가정
    });

    // ---------- 우상단 X(삭제) 오버레이 버튼 ----------
    auto *closeBtn = new QToolButton(accountBtn);
    closeBtn->setObjectName(QString("close_%1").arg(id));
    closeBtn->setText(QString::fromUtf8("X")); // 아이콘 파일 있으면 setIcon 사용
    closeBtn->setFixedSize(33, 33);
    closeBtn->setStyleSheet(
        "QToolButton { background: rgba(255,255,255,0.95); border: none; border-radius: 11px; }"

        "QToolButton:hover { background: rgba(255,80,80,0.98); color: white; }"
    );

    closeBtn->move(accountBtn->width() - closeBtn->width() - 6, 6);
    closeBtn->setVisible(m_deleteMode);  // 삭제모드일 때만 보이게

    connect(closeBtn, &QToolButton::clicked, this, [=](){
        confirmDelete(id, name);   // 안에서 Db::deleteAccount() → exitDeleteMode() → refresh 호출
    });

    // ---------- 컨테이너 구성 ----------
    auto *vbox = new QVBoxLayout;
    vbox->setAlignment(Qt::AlignHCenter);
    vbox->setContentsMargins(0,0,0,0);
    vbox->setSpacing(10);
    vbox->addWidget(accountBtn);

    QWidget *container = new QWidget;
    container->setLayout(vbox);
    container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    return container;
}

// NEW 버튼을 제외하고 모두 제거
void AccountSelectPage::clearLayout(QLayout* lay)
{
    if (!lay) return;
    while (auto item = lay->takeAt(0)) {
        if (auto w = item->widget()) w->deleteLater();
        delete item;
    }
}

// DB 기준으로 모든 계정 버튼을 다시 draw (앱 재실행 복원)
void AccountSelectPage::refreshAccountButtons()
{
    auto *layout = qobject_cast<QHBoxLayout*>(ui->accountButtonsWidget->layout());
    if (!layout) {
        qWarning() << "[Account] HBox layout not found on accountButtonsWidget";
        return;
    }

    // NEW 버튼 이외 위젯 제거 (DB 기준으로 다시 그릴 것)
    for (int i = layout->count()-1; i >= 0; --i) {
        QWidget *w = layout->itemAt(i)->widget();
        if (w && w != ui->pushButton_addNew) {
            layout->removeWidget(w);
            w->deleteLater();
        }
    }

    // DB에서 계정 목록 로드
    const auto accounts = Db::listAccounts();
    currentAccountCount = accounts.size();
    ui->pushButton_addNew->setEnabled(currentAccountCount < MAX_TOTAL_ACCOUNTS);

    // NEW 버튼 앞에 순서대로 삽입
    const QSize btnSize(250, 250);
    for (const auto& a : accounts) {
        QString img = a.avatar;
        if (img.isEmpty()) {
            img = pickRandomImagePath();           // 계정 최초 1회만
            Db::setAccountAvatar(a.id, img);       // DB에 고정
        }

        QWidget* w = this->makeAccountButtonWidget(a.name, a.id, btnSize, img);

        int insertIndex = layout->indexOf(ui->pushButton_addNew);
        if (insertIndex < 0) insertIndex = layout->count();
        layout->insertWidget(insertIndex, w);
    }
}

// "새 계정" 눌렀을 때: DB에 추가 → avatar 저장 → 화면 새로고침
void AccountSelectPage::on_pushButton_addNew_clicked()
{
    // DB 기준 현재 개수 체크
    const auto accounts = Db::listAccounts();
    if (accounts.size() >= MAX_TOTAL_ACCOUNTS)
        return;

    // 중복 없는 기본 이름 만들기 (account_0, account_1, ...)
    QSet<QString> names; for (const auto& a : accounts) names.insert(a.name);
    QString base = "account_"; int n = 0;
    while (names.contains(base + QString::number(n))) ++n;
    QString accountName = base + QString::number(n);

    int newId = -1;
    if (Db::createAccount(accountName, &newId)) {
        const QString avatar = pickRandomImagePath(); // 1회만 랜덤
        Db::setAccountAvatar(newId, avatar);          // DB에 저장
        refreshAccountButtons();                      // 즉시 반영
    } else {
        qWarning() << "[Account] create failed";
    }
}

void AccountSelectPage::enterDeleteMode()
{
    if (m_deleteMode) return;
    m_deleteMode = true;
    refreshAccountButtons();             // X버튼을 그려주기 위해 재그리기
    ui->pushButton_addNew->setEnabled(false);
}

void AccountSelectPage::exitDeleteMode()
{
    if (!m_deleteMode) return;
    m_deleteMode = false;
    refreshAccountButtons();             // X버튼 감추기
    ui->pushButton_addNew->setEnabled(true);
}

void AccountSelectPage::mousePressEvent(QMouseEvent *e)
{
    if (m_deleteMode) {
        QWidget *w = childAt(e->pos());
        // X(QToolButton)나 계정 QPushButton 이외의 곳을 누르면 해제
        if (!w || (!qobject_cast<QToolButton*>(w) && !qobject_cast<QPushButton*>(w))) {
            exitDeleteMode();
            return;
        }
    }
    QWidget::mousePressEvent(e);
}

void AccountSelectPage::keyPressEvent(QKeyEvent *e)
{
    if (m_deleteMode && e->key() == Qt::Key_Escape) {
        exitDeleteMode();
        return;
    }
    QWidget::keyPressEvent(e);
}

void AccountSelectPage::confirmDelete(int accountId, const QString& name)
{
    if (accountId <= 0) return;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("계정 삭제"));
    dlg.setModal(true);
    dlg.setMinimumSize(550, 120); // size

    // ---- Layout ----
    auto *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(24, 24, 24, 20);
    root->setSpacing(18);

    // icon + text
    auto *row = new QHBoxLayout();
    row->setSpacing(18);

    // icon
    QLabel *icon = new QLabel(&dlg);
    QPixmap pm(":/images/trash.png");
    if (pm.isNull()) pm = QPixmap(":/images/delete.png");
    if (pm.isNull()) pm = QPixmap();
    if (!pm.isNull()) icon->setPixmap(pm.scaled(56,56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    icon->setFixedSize(56,56);
    row->addWidget(icon, 0, Qt::AlignTop);

    // text column
    auto *textCol = new QVBoxLayout();
    QLabel *title = new QLabel(tr("<b>'%1'</b> 계정을 삭제할까요?").arg(name.toHtmlEscaped()), &dlg);
    title->setWordWrap(true);
    textCol->addWidget(title);
    row->addLayout(textCol, 1);

    root->addLayout(row);

    // action btn
    auto *actions = new QHBoxLayout();
    actions->addStretch();
    QPushButton *btnDelete = new QPushButton(tr("삭제"), &dlg);
    QPushButton *btnCancel = new QPushButton(tr("취소"), &dlg);
    btnCancel->setDefault(true);
    actions->addWidget(btnDelete);
    actions->addWidget(btnCancel);
    root->addLayout(actions);

    // ---- style ----
    dlg.setStyleSheet(R"(
        QDialog {
            background-color: #0f1b2c;
            border: 1px solid rgba(255,255,255,0.10);
        }
        QLabel { color: #e6edf3; }
        QLabel:nth-child(1) { font-size: 25px; font-weight: 600; }  /* title */
        QPushButton {
            min-width: 120px; min-height: 36px;
            padding: 4px 12px; border-radius: 10px;
            background: #12243a; color: #e6edf3;
            border: 1px solid rgba(255,255,255,0.14);
        }
        QPushButton:hover   { background: #1b3553; }
        QPushButton:pressed { background: #0f1b2c; }
        QPushButton#danger  {
            background: #d64545; color: white; border: none;
        }
        QPushButton#danger:hover { background: #c23b3b; }
    )");
    btnDelete->setObjectName("danger");

    // signal e
    connect(btnDelete, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    // screen aligent(Center)
    QRect parentRect = this->isWindow()
                     ? this->frameGeometry()
                     : QRect(this->mapToGlobal(QPoint(0,0)), this->size());

    QRect target = QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                       dlg.size(), parentRect);
    dlg.setGeometry(target);

    // run
    if (dlg.exec() != QDialog::Accepted) return;

    // delete
    if (!Db::deleteAccount(accountId)) {
        QMessageBox::warning(this, tr("오류"), tr("계정 삭제에 실패했습니다."));
        return;
    }
    exitDeleteMode();
    refreshAccountButtons();
}
