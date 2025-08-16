#include "game_page.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QFile>
#include <QTimer>
#include <QDebug>

#include "puzzle_pane.h"

static void clearContainer(QWidget* w) {
    if (!w) return;
    if (QLayout* lay = w->layout()) {
        while (QLayoutItem* it = lay->takeAt(0)) {
            if (QWidget* cw = it->widget()) { cw->setParent(nullptr); cw->deleteLater(); }
            if (QLayout* childLay = it->layout()) {
                while (QLayoutItem* it2 = childLay->takeAt(0)) {
                    if (QWidget* ww = it2->widget()) { ww->setParent(nullptr); ww->deleteLater(); }
                    delete it2;
                }
                delete childLay;
            }
            delete it;
        }
    } else {
        const auto direct = w->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget* c : direct) { c->setParent(nullptr); c->deleteLater(); }
    }
}

GamePage::GamePage(QWidget* selectArea,
                   QWidget* gameHost,
                   QWidget* parent)
    : QWidget(parent),
      m_selectArea(selectArea),
      m_gameHost(gameHost)
{
    clearContainer(m_selectArea);
    clearContainer(m_gameHost);

    if (!m_selectArea->layout()) {
        auto *selLay = new QVBoxLayout(m_selectArea);
        selLay->setContentsMargins(0,0,0,0);
    }
    if (!m_gameHost->layout()) {
        auto *hostLay = new QVBoxLayout(m_gameHost);
        hostLay->setContentsMargins(0,0,0,0);
//        hostLay->addWidget(new QLabel(tr("게임을 선택하세요."), m_gameHost));
    }

    m_gameHost->setObjectName("widgetGameHost");

    if (auto* hostLay = qobject_cast<QVBoxLayout*>(m_gameHost->layout())) {
        hostLay->setContentsMargins(8,8,8,8);
        hostLay->setSpacing(8);
    }

    m_gameHost->setStyleSheet(R"(
    #widgetGameHost {
      background: rgba(255,255,255,0.04);
      border: 1px solid rgba(0,238,255,0.35);
      border-radius: 12px;
    }
    #widgetGameHost * { color: #000000; } /* TeXt Black */

    /* 퍼즐 보드/조각 리스트는 2단계에서 objectName 부여 후 적용 */
    #widgetGameHost #puzzleBoard {
      background: rgba(255,255,255,0.03);
      border: 1px solid rgba(0,238,255,0.30);
      border-radius: 10px;
    }
    #widgetGameHost #puzzlePieces {
      background: transparent;
      border: 1px dashed rgba(0,238,255,0.22);
      border-radius: 8px;
    }
    #widgetGameHost #puzzlePieces::item:selected {
      background: rgba(0,238,255,0.18);
      border: 1px solid rgba(0,238,255,0.45);
    }
    )");

    buildSimpleList();
}

void GamePage::buildSimpleList()
{
    m_list = new QListWidget(m_selectArea);
    m_list->setObjectName("listWidget_GameList");
    m_selectArea->layout()->addWidget(m_list);

    m_list->setViewMode(QListView::ListMode);
    m_list->setMovement(QListView::Static);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setWrapping(false);
    m_list->setSpacing(4);
    m_list->setUniformItemSizes(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setWordWrap(true);

    m_list->setStyleSheet(R"(
        QListWidget#listWidget_GameList {
            background: rgba(255,255,255,0.04);
            border: 1px solid rgba(0,238,255,0.35);
            border-radius: 10px;
            outline: 0;
        }
        QListWidget#listWidget_GameList::item {
            height: 56px;               /* 아이템 더 크고 */
            padding: 10px 12px;
            color: #e8f0ff;             /* 흰색 텍스트 */
            font-size: 22px;            /* 글자 크게 */
        }
        QListWidget#listWidget_GameList::item:selected {
            background: rgba(0,238,255,0.18);
            border: 1px solid rgba(0,238,255,0.45);
        }
    )");

    QPalette pal = m_list->palette();
    pal.setColor(QPalette::Text, QColor("#e8f0ff"));
    m_list->setPalette(pal);

    // ===== 게임 항목 =====
    auto *itPuzzle = new QListWidgetItem(tr("Puzzle"));
    itPuzzle->setData(Qt::UserRole, static_cast<int>(GameId::Puzzle));
    m_list->addItem(itPuzzle);

    // 더블클릭/Enter로 실행
    connect(m_list, &QListWidget::itemActivated,     this, &GamePage::onItemActivated);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &GamePage::onItemActivated);
}

void GamePage::onItemActivated(QListWidgetItem* item)
{
    qDebug() << "[GP] activated item" << (item?item->text():"<null>")
             << "data=" << (item?item->data(Qt::UserRole).toInt():-999);

    if (!item) return;
    const auto id = static_cast<GameId>(item->data(Qt::UserRole).toInt());
    switchToGame(id);
}

void GamePage::switchToGame(GameId id)
{
    if (m_running == id && m_gameWidget) return;

    clearContainer(m_gameHost);

    QVBoxLayout* lay = qobject_cast<QVBoxLayout*>(m_gameHost->layout());
    if (!lay) {
        lay = new QVBoxLayout(m_gameHost);
        lay->setContentsMargins(0,0,0,0);
    }

    switch (id) {
    case GameId::Puzzle: {
        m_puzzle = new PuzzlePane(m_gameHost);
        m_gameHost->layout()->addWidget(m_puzzle);

        QTimer::singleShot(0, m_puzzle, [this]{
            m_puzzle->loadImage(QPixmap(":/images/Puzzle_Telli.png"));
        });
        break;
    }
    default: {
        auto *lbl = new QLabel(tr("아직 준비되지 않은 게임입니다."), m_gameHost);
        lbl->setAlignment(Qt::AlignCenter);
        m_gameWidget = lbl;
        lay->addWidget(m_gameWidget, 1);
        break;
    }
    }
    m_running = id;
}
