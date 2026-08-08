# -*- coding: utf-8 -*-
"""CSP 约束求解器 baseline。

阶段 1：数字格约束推导（旗子数=数字 -> 其余隐藏格安全；旗子+隐藏=数字 -> 全雷）；
阶段 2：无必然解时按“最小邻域雷密度”猜测。

返回 ("reveal"|"flag", idx) 供 evaluate.py 使用；后续可把它当作监督学习的老师。
"""


def solve_step(env):
    rows, cols = env.rows, env.cols
    grid = env._grid
    revealed = env._revealed
    flagged = env._flagged

    def hidden_info(r, c):
        hidden, flag_count = [], 0
        for nr, nc in env._neighbors(r, c):
            if not revealed[nr, nc] and not flagged[nr, nc]:
                hidden.append((nr, nc))
            elif flagged[nr, nc]:
                flag_count += 1
        return hidden, flag_count

    # 阶段 1：确定性推理
    for r in range(rows):
        for c in range(cols):
            if not revealed[r, c] or grid[r, c] <= 0:
                continue
            n = int(grid[r, c])
            hidden, flag_count = hidden_info(r, c)
            if flag_count == n and hidden:
                hr, hc = hidden[0]
                return ("reveal", hr * cols + hc)
            if flag_count + len(hidden) == n and hidden:
                hr, hc = hidden[0]
                return ("flag", hr * cols + hc)

    # 阶段 2：概率猜测（最小邻域雷密度）
    hidden_all = env.valid_actions()
    if not hidden_all:
        return None
    mines_left = env.mines - int(flagged.sum())
    best, best_risk = None, None
    for idx in hidden_all:
        r, c = divmod(idx, cols)
        risk = None
        for nr, nc in env._neighbors(r, c):
            if revealed[nr, nc] and grid[nr, nc] > 0:
                n = int(grid[nr, nc])
                hh, ff = hidden_info(nr, nc)
                if hh:
                    v = (n - ff) / len(hh)
                    risk = v if risk is None else min(risk, v)
        if risk is None:
            risk = mines_left / len(hidden_all)
        if best_risk is None or risk < best_risk:
            best_risk, best = risk, idx
    return ("reveal", best)
