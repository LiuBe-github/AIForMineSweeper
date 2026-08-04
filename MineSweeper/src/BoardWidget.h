#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>

#include "MineField.h"

// 整个棋盘用一个控件自绘：大棋盘不再创建海量子控件，显著降低内存与刷新开销。
// 格子尺寸随棋盘大小自适应，超大棋盘自动缩小格子，保证窗口不超出屏幕。
class BoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit BoardWidget(MineField* field, QWidget* parent = nullptr);

    int cellSize() const { return cellPx_; }

    // 只重绘发生变化的格子；变化过多时退化为整板刷新
    void refresh(const std::vector<int>& dirtyCells);

signals:
    void leftClicked(int row, int col);
    void rightClicked(int row, int col);
    void chordRequested(int row, int col);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QRect cellRect(int row, int col) const;
    void paintCell(QPainter& p, int row, int col);

    MineField* field_ = nullptr;
    int cellPx_ = 28;
};

#endif  // BOARDWIDGET_H
