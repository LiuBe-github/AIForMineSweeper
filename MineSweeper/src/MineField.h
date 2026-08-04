#ifndef MINEFIELD_H
#define MINEFIELD_H

#include <vector>

// 单个格子的显示状态
enum class CellState {
    Hidden,    // 未翻开
    Revealed,  // 已翻开
    Flagged    // 已插旗
};

struct Cell {
    bool mine = false;        // 是否是地雷
    int adjacent = 0;         // 相邻地雷数量（0~8）
    bool wrongFlag = false;   // 失败时，插错旗的格子显示红叉
    bool detonated = false;   // 失败时，被点击引爆的那颗雷
    CellState state = CellState::Hidden;
};

// 扫雷核心逻辑（不依赖 Qt，便于单独测试）
class MineField {
public:
    MineField() = default;
    MineField(int rows, int cols, int mines);

    void reset(int rows, int cols, int mines);
    void reset();  // 用当前参数重新开局

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int mineCount() const { return mines_; }
    int flaggedCount() const { return flagged_; }
    int revealedCount() const { return revealed_; }
    bool isFirstClickPending() const { return !firstClickDone_; }
    bool isGameOver() const { return gameOver_; }
    bool isWon() const { return won_; }

    const Cell& cell(int row, int col) const { return grid_[row][col]; }
    bool inBounds(int row, int col) const;

    // 返回自上次调用以来状态发生变化的格子索引（r*cols+c），并清空记录；
    // 供界面只重绘变化的格子，大棋盘下显著降低刷新开销。
    std::vector<int> takeDirty();

    // 左键翻开；首次点击时布雷，保证首格（及周围）安全
    void reveal(int row, int col);
    // 右键插旗/取消旗
    void toggleFlag(int row, int col);
    // 数字格快速翻开周围（双击或中键）
    void chord(int row, int col);

private:
    void placeMines(int safeRow, int safeCol);
    void computeAdjacency();
    void floodReveal(int row, int col);
    void revealAllMines();
    void checkWin();
    void markDirty(int row, int col);

    std::vector<std::vector<Cell>> grid_;
    std::vector<int> dirtyCells_;
    int rows_ = 0;
    int cols_ = 0;
    int mines_ = 0;
    int revealed_ = 0;
    int flagged_ = 0;
    bool firstClickDone_ = false;
    bool gameOver_ = false;
    bool won_ = false;
};

#endif  // MINEFIELD_H
