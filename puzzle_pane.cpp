#include "puzzle_pane.h"
#include <QtWidgets>

#include "puzzlewidget.h"
#include "piecesmodel.h"

PuzzlePane::PuzzlePane(QWidget* parent) : QWidget(parent) {
    setupUi();
    setupModelAndList();
    startNewGame(); // 기본 이미지로 새 게임 시작
}

void PuzzlePane::setupUi() {
    puzzle = new PuzzleWidget(325, this);     // 정사각 325px 보드
    puzzle->setObjectName("puzzleBoard");
    puzzle->setAttribute(Qt::WA_StyledBackground, true);
    btnRestart = new QPushButton(tr("Restart"), this);
    btnOpen    = new QPushButton(tr("Open Image..."), this);
    btnRestart->setFocusPolicy(Qt::NoFocus);
    btnOpen->setFocusPolicy(Qt::NoFocus);

    /* btn size */
    btnRestart->setMinimumSize(120, 40);
    btnOpen->setMinimumSize(150, 40);

    btnRestart->setStyleSheet("font-size: 18px; padding: 8px 16px;");
    btnOpen->setStyleSheet("font-size: 18px; padding: 8px 16px;");

    connect(btnRestart, &QPushButton::clicked, this, &PuzzlePane::onRestart);
    connect(btnOpen,    &QPushButton::clicked, this, &PuzzlePane::onOpenFile);
    connect(puzzle,     &PuzzleWidget::puzzleCompleted, this, &PuzzlePane::onCompleted,
            Qt::QueuedConnection); // 완료 시 다시 새 게임 등

    auto *list = new QListView(this);  // 나중에 piecesList로 교체
    piecesList = list;
    piecesList->setObjectName("puzzlePieces");
    piecesList->setFrameShape(QFrame::NoFrame);
    piecesList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *g = new QGridLayout(this);
    g->setContentsMargins(0,0,0,0);
    g->setSpacing(8);
    g->addWidget(piecesList, 0,0, 3,1);
    g->addWidget(puzzle,     0,1, 3,1);
    g->addWidget(btnRestart, 3,1, 1,1, Qt::AlignRight);
    g->addWidget(btnOpen,    3,0, 1,1, Qt::AlignLeft);
}

void PuzzlePane::setupModelAndList() {
    model = new PiecesModel(puzzle->pieceSize(), this);
    piecesList->setModel(model);

    // 리스트 설정(아이콘/격자/드래그/드롭)
    piecesList->setDragEnabled(true);
    piecesList->setViewMode(QListView::IconMode);
    piecesList->setIconSize(QSize(puzzle->pieceSize()-20, puzzle->pieceSize()-20));
    piecesList->setGridSize(QSize(puzzle->pieceSize(),   puzzle->pieceSize()));
    piecesList->setSpacing(10);
    piecesList->setMovement(QListView::Snap);
    piecesList->setAcceptDrops(true);
    piecesList->setDropIndicatorShown(true);
}

void PuzzlePane::startNewGame() {
    // 기본 이미지를 코드로 생성(리소스가 없어도 동작하도록)
    QPixmap pm(puzzle->imageSize(), puzzle->imageSize());
    pm.fill(Qt::white);
    QPainter p(&pm);

    QLinearGradient grad(0,0, pm.width(), pm.height());
    grad.setColorAt(0, QColor(200,230,255));
    grad.setColorAt(1, QColor(180,210,255));
    p.fillRect(pm.rect(), grad);
    p.setPen(QPen(Qt::darkBlue, 6));
    p.drawRect(pm.rect().adjusted(3,3,-3,-3));
    p.setPen(QPen(Qt::blue, 2));
    for (int i=1;i<5;++i) {
        int s = pm.width()/5;
        p.drawLine(i*s,0, i*s, pm.height());
        p.drawLine(0,i*s, pm.width(), i*s);
    }
    p.end();

    loadImage(pm);
}

void PuzzlePane::loadImage(const QPixmap& src)
{
    if (src.isNull()) return;

    const int s = puzzle->pieceSize() * 5;
    QPixmap scaled = src.scaled(s, s, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap canvas(s, s);
    canvas.fill(Qt::transparent);

    {
        QPainter p(&canvas);
        const int x = (s - scaled.width())  / 2;
        const int y = (s - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
    }

    image = canvas;
    model->addPieces(canvas);
}

void PuzzlePane::rebuildPieces() {
    if (image.isNull()) {
        startNewGame();
        return;
    }
    model->addPieces(image);
}

void PuzzlePane::onRestart() {
    rebuildPieces();
    puzzle->clear();
}

void PuzzlePane::onOpenFile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Image"), QString(),
                             tr("Image Files (*.png *.jpg *.bmp)"));
    if (path.isEmpty()) return;
    QPixmap pm;
    if (!pm.load(path)) return;

    const int size = qMin(pm.width(), pm.height());
    pm = pm.copy((pm.width()-size)/2, (pm.height()-size)/2, size, size)
         .scaled(puzzle->imageSize(), puzzle->imageSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    loadImage(pm);
}

void PuzzlePane::onCompleted() {
    QMessageBox::information(this, tr("Puzzle"), tr("Completed! New game will start."), QMessageBox::Ok);
    startNewGame();
}
