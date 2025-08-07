#include "accountselectpage.h"
#include "ui_accountselectpage.h"

#include <QPushButton>
#include <QDebug>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QProcess>
#include <QLineEdit>

#include <QPainter>

AccountSelectPage::AccountSelectPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AccountSelectPage)
{
    ui->setupUi(this);

    QString btnStyle = R"(
        QPushButton {
            background-color: #124;
            color: white;
            font-size: 16pt;
            border-radius: 15px;
            padding: 10px;
        }
        QPushButton:hover {
            background-color: #666;
        }
    )";
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    ui->pushButton_addNew->setStyleSheet(btnStyle);

    this->btnStyleSheet = btnStyle;
}

void AccountSelectPage::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    QPixmap bg(":/images/account_background.png");  // Qt 리소스 경로

    if (!bg.isNull()) {
        painter.drawPixmap(this->rect(), bg);
    }
}

AccountSelectPage::~AccountSelectPage()
{
    delete ui;
}

void AccountSelectPage::on_pushButton_addNew_clicked()
{
    if (currentAccountCount >= MAX_TOTAL_ACCOUNTS)
        return;

    QString accountName = QString("account_%1").arg(currentAccountCount);

    // (1) 버튼 생성
    QPushButton *accountBtn = new QPushButton(QString("").arg(currentAccountCount));
    accountBtn->setObjectName(accountName);
    accountBtn->setFixedSize(250, 250);

    QStringList accountImagePaths = {
        ":/account/images/Angry.png",
        ":/account/images/Bob.png",
        ":/account/images/Graph.png",
        ":/account/images/killer.png",
        ":/account/images/Rabbit.png",
        ":/account/images/Robot.png",
        ":/account/images/Sheep.png",
        ":/account/images/who.png",\
    };

    int randomIndex = QRandomGenerator::global()->bounded(accountImagePaths.size());
    QString imagePath = accountImagePaths[randomIndex];

    QString btnStyleWithImage = QString(R"(
        QPushButton {
            background-image: url(%1);
            background-repeat: no-repeat;
            background-position: center;
            background-size: contain;
            background-color: #124;
            color: white;
            font-size: 16pt;
            border-radius: 15px;
            padding: 10px;
        }
        QPushButton:hover {
            background-color: #666;
        }
    )").arg(imagePath);

    accountBtn->setStyleSheet(btnStyleWithImage);


    // (3) 수직 정렬로 버튼 + 입력창 묶음
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setAlignment(Qt::AlignHCenter);
    vbox->setSpacing(10);
    vbox->addWidget(accountBtn);

    QWidget *container = new QWidget;
    container->setLayout(vbox);
    container->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    // (4) 기존 레이아웃에 추가 (NEW 버튼 앞에)
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(ui->accountButtonsWidget->layout());
    if (!layout) {
        qDebug() << "[ERROR] layout is not HBoxLayout";
        return;
    }

    int insertIndex = layout->indexOf(ui->pushButton_addNew);
    layout->insertWidget(insertIndex, container);  // NEW 버튼 앞에 삽입

    currentAccountCount++;

    // (6) 버튼 클릭 시 선택 시그널 전송
    connect(accountBtn, &QPushButton::clicked, this, [=]() {
        emit accountSelected(accountName);
    });
}

