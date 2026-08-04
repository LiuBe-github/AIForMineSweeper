#include "MineField.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

MineField::MineField(int rows, int cols, int mines) {
    reset(rows, cols, mines);
}

void MineField::reset(int rows, int cols, int mines) {
    rows_ = rows;
    cols_ = cols;
    const int maxMines = rows * cols - 1;
    mines_ = std::clamp(mines, 0, maxMines > 0 ? maxMines : 0);
    grid_.assign(rows_, std::vector<Cell>(cols_));
    revealed_ = 0;
    flagged_ = 0;
    firstClickDone_ = false;
    gameOver_ = false;
    won_ = false;
}

void MineField::reset() {
    reset(rows_, cols_, mines_);
}

bool MineField::inBounds(int row, int col) const {
    return row >= 0 && row < rows_ && col >= 0 && col < cols_;
}

void MineField::placeMines(int safeRow, int safeCol) {
    // 可布雷的格子：默认排除首次点击的 3x3 区域，让第一下有更好的体验
    std::vector<std::pair<int, int>> candidates;
    candidates.reserve(rows_ * cols_);
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (std::abs(r - safeRow) <= 1 && std::abs(c - safeCol) <= 1)
                continue;
            candidates.emplace_back(r, c);
        }
    }
    // 棋盘太小时退化为只排除点击格本身
    if (static_cast<int>(candidates.size()) < mines_) {
        candidates.clear();
        for (int r = 0; r < rows_; ++r)
            for (int c = 0; c < cols_; ++c)
                if (r != safeRow || c != safeCol)
                    candidates.emplace_back(r, c);
    }

    std::shuffle(candidates.begin(), candidates.end(),
                 std::mt19937(std::random_device{}()));
    for (int i = 0; i < mines_; ++i)
        grid_[candidates[i].first][candidates[i].second].mine = true;

    computeAdjacency();
}

void MineField::computeAdjacency() {
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c].mine)
                continue;
            int n = 0;
            for (int dr = -1; dr <= 1; ++dr) {
                for (int dc = -1; dc <= 1; ++dc) {
                    if (dr == 0 && dc == 0)
                        continue;
                    const int nr = r + dr;
                    const int nc = c + dc;
                    if (inBounds(nr, nc) && grid_[nr][nc].mine)
                        ++n;
                }
            }
            grid_[r][c].adjacent = n;
        }
    }
}

void MineField::reveal(int row, int col) {
    if (!inBounds(row, col) || gameOver_)
        return;

    Cell& cell = grid_[row][col];
    // 已翻开或已插旗的格子不响应左键
    if (cell.state != CellState::Hidden)
        return;

    if (!firstClickDone_) {
        firstClickDone_ = true;
        placeMines(row, col);
    }

    if (cell.mine) {
        cell.state = CellState::Revealed;
        cell.detonated = true;
        gameOver_ = true;
        won_ = false;
        revealAllMines();
        return;
    }

    if (cell.adjacent == 0)
        floodReveal(row, col);
    else {
        cell.state = CellState::Revealed;
        ++revealed_;
    }
    checkWin();
}

void MineField::floodReveal(int row, int col) {
    if (!inBounds(row, col))
        return;
    Cell& cell = grid_[row][col];
    if (cell.state != CellState::Hidden || cell.mine)
        return;

    cell.state = CellState::Revealed;
    ++revealed_;

    // 数字格停止扩散
    if (cell.adjacent != 0)
        return;

    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc)
            if (dr != 0 || dc != 0)
                floodReveal(row + dr, col + dc);
}

void MineField::toggleFlag(int row, int col) {
    if (!inBounds(row, col) || gameOver_)
        return;

    Cell& cell = grid_[row][col];
    if (cell.state == CellState::Revealed)
        return;
    if (cell.state == CellState::Hidden) {
        cell.state = CellState::Flagged;
        ++flagged_;
    } else {
        cell.state = CellState::Hidden;
        --flagged_;
    }
}

void MineField::chord(int row, int col) {
    if (!inBounds(row, col) || gameOver_)
        return;

    const Cell& cell = grid_[row][col];
    if (cell.state != CellState::Revealed || cell.adjacent == 0)
        return;

    // 周围旗子数量必须与数字相等才允许快速翻开
    int flags = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0)
                continue;
            const int nr = row + dr;
            const int nc = col + dc;
            if (inBounds(nr, nc) && grid_[nr][nc].state == CellState::Flagged)
                ++flags;
        }
    }
    if (flags != cell.adjacent)
        return;

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0)
                continue;
            const int nr = row + dr;
            const int nc = col + dc;
            if (!inBounds(nr, nc))
                continue;

            Cell& n = grid_[nr][nc];
            if (n.state != CellState::Hidden)
                continue;
            if (n.mine) {
                n.state = CellState::Revealed;
                n.detonated = true;
                gameOver_ = true;
                won_ = false;
                revealAllMines();
                return;
            }
            if (n.adjacent == 0)
                floodReveal(nr, nc);
            else {
                n.state = CellState::Revealed;
                ++revealed_;
            }
        }
    }
    checkWin();
}

void MineField::revealAllMines() {
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            Cell& cell = grid_[r][c];
            // 已正确插旗的雷保留旗子
            if (cell.mine && cell.state == CellState::Flagged)
                continue;
            if (cell.mine) {
                cell.state = CellState::Revealed;
                continue;
            }
            // 插错的旗显示为红叉
            if (cell.state == CellState::Flagged) {
                cell.wrongFlag = true;
                cell.state = CellState::Revealed;
                --flagged_;
            }
        }
    }
}

void MineField::checkWin() {
    if (revealed_ != rows_ * cols_ - mines_)
        return;
    gameOver_ = true;
    won_ = true;
    // 胜利后自动为剩余地雷插旗
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (grid_[r][c].mine && grid_[r][c].state == CellState::Hidden) {
                grid_[r][c].state = CellState::Flagged;
                ++flagged_;
            }
        }
    }
}
