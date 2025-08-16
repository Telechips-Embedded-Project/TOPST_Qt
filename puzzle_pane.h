#ifndef PUZZLE_PANE_H
#define PUZZLE_PANE_H

#pragma once
#include <QWidget>
#include <QPixmap>

class QListView;
class PiecesModel;
class PuzzleWidget;
class QPushButton;

class PuzzlePane : public QWidget {
    Q_OBJECT
public:
    explicit PuzzlePane(QWidget* parent = nullptr);

public slots:
    void startNewGame();                 // attachGame()에서 공통으로 호출될 슬롯 이름
    void loadImage(const QPixmap& img);  // 외부에서 이미지 주입용(선택)

private slots:
    void onRestart();
    void onOpenFile();                   // 데스크톱에서만 사용(임베디드면 생략 가능)
    void onCompleted();

private:
    void setupUi();
    void setupModelAndList();
    void rebuildPieces();                // model에 5x5로 조각 생성

    // 위젯/모델
    PuzzleWidget* puzzle{};              // 보드 위젯(드래그&드롭, 완료 시그널 emit)
    QListView*    piecesList{};
    PiecesModel*  model{};

    // 상태
    QPixmap       image;                 // 현재 퍼즐 원본(정사각형으로 리사이즈됨)
    QPushButton * btnRestart{}, *btnOpen{};
};

#endif // PUZZLE_PANE_H
