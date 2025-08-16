#ifndef GAME_PAGE_H
#define GAME_PAGE_H

#include <QWidget>

class QListWidget;
class QListWidgetItem;
class PuzzlePane;

class GamePage : public QWidget
{
    Q_OBJECT
public:
    enum class GameId { None = -1, Puzzle = 0 /* 확장 예정 */ };

    // .ui의 두 영역을 그대로 넘겨받아 사용
    explicit GamePage(QWidget* selectArea,
                      QWidget* gameHost,
                      QWidget* parent = nullptr);

private slots:
    void onItemActivated(QListWidgetItem* item);  // 더블클릭/Enter

private:
    void buildSimpleList();       // widget_game_select 안에 리스트 구성
    void switchToGame(GameId id); // widget_game에 게임 띄우기

    QWidget*    m_selectArea = nullptr;  // ui->widget_game_select
    QWidget*    m_gameHost   = nullptr;  // ui->widget_game
    QListWidget* m_list      = nullptr;  // 게임 선택 리스트
    QWidget*    m_gameWidget = nullptr;  // 현재 실행 중 게임 위젯
    GameId      m_running    = GameId::None;

    // Games (Puzzle, ...)
    PuzzlePane* m_puzzle     = nullptr;
};

#endif // GAME_PAGE_H
