#ifndef CELLBUTTON_H
#define CELLBUTTON_H

#include <QPushButton>

#include "MineField.h"

// 棋盘上的单个格子，负责绘制并发送鼠标事件
class CellButton : public QPushButton {
    Q_OBJECT
public:
    explicit CellButton(int row, int col, QWidget* parent = nullptr);

    void setMineField(MineField* field) { field_ = field; }
    void refresh() { update(); }

signals:
    void leftClicked(int row, int col);
    void rightClicked(int row, int col);
    void chordRequested(int row, int col);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int row_;
    int col_;
    MineField* field_ = nullptr;
};

#endif  // CELLBUTTON_H
