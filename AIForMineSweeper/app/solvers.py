# -*- coding: utf-8 -*-
"""求解入口：按 engine 分发；以及内置启发式求解器（网页默认模型）。"""

import time
import sys

from .api_client import call_deepseek, parse_move
from .config import load_config
from .validation import validate_move

ENGINE_API = "api"              # DeepSeek，使用用户提供的 API Key
ENGINE_HEURISTIC = "heuristic"  # 内置求解器，无需 API Key

_HEURISTIC_ALIASES = ("heuristic", "web", "builtin", "local",
                      "dry-run", "dry_run")


def resolve_engine(payload, cfg=None, force_heuristic=False):
    if force_heuristic:
        return ENGINE_HEURISTIC
    cfg = cfg or load_config()
    engine = str(payload.get("engine") or cfg.get("default_engine") or ENGINE_API)
    engine = engine.strip().lower()
    if engine in _HEURISTIC_ALIASES:
        return ENGINE_HEURISTIC
    return ENGINE_API


def solve_payload(payload, cfg=None, force_heuristic=False):
    """统一求解入口：--dry-run 或 engine=heuristic 走内置求解器，否则走 DeepSeek。
    返回的走法附带 engine/model 字段，供客户端记录实际使用的 AI 信息。"""
    cfg = cfg or load_config()
    engine = resolve_engine(payload, cfg, force_heuristic)
    if engine == ENGINE_HEURISTIC:
        move = solve_heuristic(payload)
        move["engine"] = "heuristic"
        move["model"] = "builtin-heuristic"
    else:
        move = solve_with_deepseek(payload, cfg)
        move["engine"] = "api"
        move["model"] = str(cfg.get("model") or "deepseek-chat")
    return move


def solve_with_deepseek(payload, cfg=None):
    cfg = cfg or load_config()
    # 混合策略：能确定性推理（安全翻开 / 必定的雷插旗 / chord）时直接返回，
    # 不调用大模型，既快又稳；只有真正需要“猜”的局面才走 DeepSeek。
    sure_move, is_sure = _find_deterministic_move(payload)
    if is_sure:
        return sure_move

    max_retries = max(1, int(cfg.get("max_retries", 3)))
    last_err = ""
    for attempt in range(max_retries):
        try:
            content = call_deepseek(cfg, payload)
            move = parse_move(content)
            ok, err = validate_move(move, payload)
            if ok and move.get("action") == "flag":
                # 猜局阶段不允许插旗：确定性推理已覆盖必定是雷的格子，
                # 大模型“猜的旗”会让当局永远无法通关。
                ok = False
                err = ("flag is only allowed when the cell is provably a mine; "
                       "in a guess situation please return reveal or chord instead")
            if not ok:
                last_err = err
                payload = dict(payload)
                payload["feedback"] = err
                continue
            return move
        except Exception as e:
            last_err = str(e)
            if attempt + 1 < max_retries:
                time.sleep(1.5 * (attempt + 1))
    # 重试耗尽后不再报错：回退到本地启发式求解，保证游戏始终能拿到合法走法。
    print("[WARN] DeepSeek 求解失败（已尝试 %d 次），回退本地求解: %s"
          % (max_retries, last_err), file=sys.stderr)
    return solve_heuristic(payload)


def _find_deterministic_move(payload):
    """确定性推理：存在安全走法时返回 (move, True)，否则 (None, False)。
    覆盖：旗数等于数字时翻开剩余安全格（chord 提速）、以及某数字周围
    未翻开格全为雷时插旗。
    """
    rows = int(payload["rows"])
    cols = int(payload["cols"])
    cells = payload["cells"]

    def get(r, c):
        if 0 <= r < rows and 0 <= c < cols:
            return cells[r * cols + c]
        return None

    for r in range(rows):
        for c in range(cols):
            cell = get(r, c)
            if not cell or cell.get("state") != "revealed":
                continue
            n = cell.get("adjacent", 0)
            if n <= 0:
                continue
            hidden = []
            flagged = 0
            for dr in (-1, 0, 1):
                for dc in (-1, 0, 1):
                    if dr == 0 and dc == 0:
                        continue
                    nb = get(r + dr, c + dc)
                    if nb is None:
                        continue
                    if nb.get("state") == "hidden":
                        hidden.append((r + dr, c + dc))
                    elif nb.get("state") == "flagged":
                        flagged += 1
            if flagged == n and hidden:
                hr, hc = hidden[0]
                return ({"action": "reveal", "row": hr, "col": hc,
                         "reason": "chord: flags match number"}, True)
            if flagged + len(hidden) == n and hidden:
                hr, hc = hidden[0]
                return ({"action": "flag", "row": hr, "col": hc,
                         "reason": "all remaining neighbors are mines"}, True)
    return None, False


def solve_heuristic(payload):
    """内置启发式策略：确定性推理 + 概率估计，不调用任何外部模型。"""
    rows = int(payload["rows"])
    cols = int(payload["cols"])
    cells = payload["cells"]

    def get(r, c):
        if 0 <= r < rows and 0 <= c < cols:
            return cells[r * cols + c]
        return None

    # 1) 确定性推理
    sure, is_sure = _find_deterministic_move(payload)
    if is_sure:
        return sure

    # 2) 概率估计：选择最不可能有雷的隐藏格
    mines_left = int(payload.get("mines", 0)) - sum(
        1 for c in cells if c.get("state") == "flagged")
    hidden_count = sum(1 for c in cells if c.get("state") == "hidden")
    best = None
    best_key = None
    for r in range(rows):
        for c in range(cols):
            cell = get(r, c)
            if not cell or cell.get("state") != "hidden":
                continue
            risk = None
            for dr in (-1, 0, 1):
                for dc in (-1, 0, 1):
                    if dr == 0 and dc == 0:
                        continue
                    nb = get(r + dr, c + dc)
                    if nb and nb.get("state") == "revealed" and nb.get("adjacent", 0) > 0:
                        n = nb.get("adjacent", 0)
                        hh = 0
                        ff = 0
                        for dr2 in (-1, 0, 1):
                            for dc2 in (-1, 0, 1):
                                if dr2 == 0 and dc2 == 0:
                                    continue
                                nb2 = get(r + dr + dr2, c + dc + dc2)
                                if nb2 is None:
                                    continue
                                if nb2.get("state") == "hidden":
                                    hh += 1
                                elif nb2.get("state") == "flagged":
                                    ff += 1
                        if hh > 0:
                            v = (n - ff) / hh
                            risk = v if risk is None else min(risk, v)
            if risk is None:
                # 未接触数字的盲区：按剩余雷数/剩余隐藏格数估计
                risk = (mines_left / hidden_count) if hidden_count else 1.0
            key = (risk, abs(r - rows // 2) + abs(c - cols // 2), r, c)
            if best_key is None or key < best_key:
                best_key = key
                best = (r, c)
    if best:
        return {"action": "reveal", "row": best[0], "col": best[1],
                "reason": "guess: lowest estimated mine risk"}

    # 3) 兜底
    return {"action": "reveal", "row": 0, "col": 0, "reason": "fallback"}
