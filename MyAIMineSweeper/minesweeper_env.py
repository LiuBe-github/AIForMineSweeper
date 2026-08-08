# -*- coding: utf-8 -*-
"""扫雷强化学习环境（规则与 C++ 版 MineField 对齐）。

规则要点：
- 首次点击安全：布雷时避开首格及其 3x3 邻域；
- 翻开 0 格时洪泛展开；
- 翻开所有非雷格即胜利（RL 版不需要插旗）；
- 踩雷即失败。

状态表示（numpy, float32, shape=(4, H, W)）：
- ch0: 已翻开的数字 / 8
- ch1: 旗子 (1.0)
- ch2: 隐藏格 (1.0)
- ch3: 剩余雷数 / 总雷数（常数通道）

动作：
- int：0..rows*cols-1，表示“翻开”该格；
- ("flag", idx)：插旗/取消旗（供 CSP baseline 与后续扩展使用）。
"""

import numpy as np


class MinesweeperEnv:
    def __init__(self, rows=9, cols=9, mines=10, seed=None):
        self.rows = rows
        self.cols = cols
        self.mines = mines
        self.rng = np.random.default_rng(seed)
        self.reset()

    # ------------------------------------------------------------------ #
    # 基础 API
    # ------------------------------------------------------------------ #
    def reset(self, seed=None):
        if seed is not None:
            self.rng = np.random.default_rng(seed)
        self._grid = np.zeros((self.rows, self.cols), dtype=np.int8)  # -1 雷，0..8 数字
        self._revealed = np.zeros((self.rows, self.cols), dtype=bool)
        self._flagged = np.zeros((self.rows, self.cols), dtype=bool)
        self._first_click_done = False
        self._game_over = False
        self._won = False
        self._revealed_count = 0
        return self.state()

    def step(self, action):
        if self._game_over:
            return self.state(), 0.0, True, {"game_over": True}
        if isinstance(action, tuple):
            kind, idx = action
            if kind == "flag":
                return self._step_flag(int(idx))
            return self._step_reveal(int(idx))
        return self._step_reveal(int(action))

    def state(self):
        s = np.zeros((4, self.rows, self.cols), dtype=np.float32)
        revealed_nums = np.where(self._revealed,
                                 self._grid.astype(np.float32) / 8.0, 0.0)
        s[0] = revealed_nums
        s[1] = self._flagged.astype(np.float32)
        s[2] = (~self._revealed & ~self._flagged).astype(np.float32)
        s[3] = np.full((self.rows, self.cols),
                       self.mines_remaining / max(self.mines, 1),
                       dtype=np.float32)
        return s

    def valid_actions(self):
        """当前所有可“翻开”的格子索引（隐藏且未插旗）。"""
        return [r * self.cols + c
                for r in range(self.rows)
                for c in range(self.cols)
                if not self._revealed[r, c] and not self._flagged[r, c]]

    def render(self):
        lines = []
        for r in range(self.rows):
            line = []
            for c in range(self.cols):
                if self._flagged[r, c]:
                    line.append("F")
                elif not self._revealed[r, c]:
                    line.append(".")
                elif self._grid[r, c] == -1:
                    line.append("*")
                else:
                    line.append(str(int(self._grid[r, c])))
            lines.append(" ".join(line))
        return "\n".join(lines)

    # ------------------------------------------------------------------ #
    # 内部实现
    # ------------------------------------------------------------------ #
    @property
    def mines_remaining(self):
        return self.mines - int(self._flagged.sum())

    def _in_bounds(self, r, c):
        return 0 <= r < self.rows and 0 <= c < self.cols

    def _neighbors(self, r, c):
        out = []
        for dr in (-1, 0, 1):
            for dc in (-1, 0, 1):
                if dr == 0 and dc == 0:
                    continue
                nr, nc = r + dr, c + dc
                if self._in_bounds(nr, nc):
                    out.append((nr, nc))
        return out

    def _place_mines(self, safe_r, safe_c):
        safe = {(safe_r, safe_c)}
        safe.update(self._neighbors(safe_r, safe_c))
        cells = [(r, c) for r in range(self.rows) for c in range(self.cols)
                 if (r, c) not in safe]
        if len(cells) < self.mines:
            raise ValueError("棋盘太小，无法在安全区外放置 %d 颗雷" % self.mines)
        self.rng.shuffle(cells)
        for r, c in cells[:self.mines]:
            self._grid[r, c] = -1
        for r in range(self.rows):
            for c in range(self.cols):
                if self._grid[r, c] == -1:
                    continue
                self._grid[r, c] = sum(
                    1 for nr, nc in self._neighbors(r, c)
                    if self._grid[nr, nc] == -1)

    def reveal(self, r, c):
        """内部翻开逻辑；返回 (ok, 消息)。"""
        if not self._in_bounds(r, c):
            return False, "越界"
        if self._revealed[r, c] or self._flagged[r, c]:
            return False, "不是隐藏格"
        if not self._first_click_done:
            self._place_mines(r, c)
            self._first_click_done = True
        if self._grid[r, c] == -1:
            self._revealed[r, c] = True
            self._revealed_count += 1
            self._game_over = True
            return True, "踩雷"
        self._revealed[r, c] = True
        self._revealed_count += 1
        if self._grid[r, c] == 0:
            self._flood_reveal(r, c)
        if self._revealed_count == self.rows * self.cols - self.mines:
            self._won = True
            self._game_over = True
        return True, ""

    def _flood_reveal(self, r, c):
        stack = [(r, c)]
        while stack:
            cr, cc = stack.pop()
            for nr, nc in self._neighbors(cr, cc):
                if self._revealed[nr, nc] or self._flagged[nr, nc]:
                    continue
                self._revealed[nr, nc] = True
                self._revealed_count += 1
                if self._grid[nr, nc] == 0:
                    stack.append((nr, nc))

    def _step_reveal(self, idx):
        before = self._revealed_count
        r, c = divmod(idx, self.cols)
        ok, msg = self.reveal(r, c)
        if not ok:
            return self.state(), -0.5, False, {"invalid": True, "msg": msg}
        if self._won:
            return self.state(), 10.0, True, {"won": True}
        if self._game_over:
            return self.state(), -1.0, True, {"lost": True}
        gained = self._revealed_count - before
        reward = 0.05 + 0.02 * min(gained, 10)
        return self.state(), reward, False, {"revealed": gained}

    def _step_flag(self, idx):
        r, c = divmod(idx, self.cols)
        if not self._in_bounds(r, c):
            return self.state(), -0.5, False, {"invalid": True, "msg": "越界"}
        if self._revealed[r, c]:
            return self.state(), -0.5, False, {"invalid": True, "msg": "已翻开"}
        self._flagged[r, c] = not self._flagged[r, c]
        return self.state(), -0.05, False, {"flagged": bool(self._flagged[r, c])}
