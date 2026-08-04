# -*- coding: utf-8 -*-
"""棋盘载荷与 AI 返回结果的校验。"""


def validate_move(move, payload):
    rows = int(payload["rows"])
    cols = int(payload["cols"])
    r, c = move["row"], move["col"]
    if not (0 <= r < rows and 0 <= c < cols):
        return False, "坐标 (%d,%d) 超出棋盘" % (r, c)
    cell = payload["cells"][r * cols + c]
    state = cell.get("state")
    action = move["action"]
    if action == "reveal" and state != "hidden":
        return False, "格子 (%d,%d) 不是隐藏格，不能翻开" % (r, c)
    if action == "flag" and state not in ("hidden", "flagged"):
        return False, "格子 (%d,%d) 已翻开，不能插旗" % (r, c)
    if action == "chord":
        if state != "revealed" or cell.get("adjacent", 0) <= 0:
            return False, "格子 (%d,%d) 不是带数字的已翻开格，不能 chord" % (r, c)
    return True, ""


def validate_payload(payload):
    """校验棋盘载荷的基本结构，返回 (ok, 错误信息)。"""
    try:
        rows = int(payload["rows"])
        cols = int(payload["cols"])
        mines = int(payload.get("mines", 0))
        cells = payload["cells"]
    except (KeyError, TypeError, ValueError):
        return False, "载荷缺少 rows/cols/mines/cells 字段"
    if rows <= 0 or cols <= 0:
        return False, "rows/cols 必须为正数"
    if not isinstance(cells, list):
        return False, "cells 必须是数组"
    if len(cells) != rows * cols:
        return False, "cells 数量 (%d) 与棋盘大小 rows*cols (%d) 不一致" % (
            len(cells), rows * cols)
    if mines < 0 or mines >= rows * cols:
        return False, "mines 数量不合法"
    for cell in cells:
        if not isinstance(cell, dict) or cell.get("state") not in (
                "hidden", "revealed", "flagged"):
            return False, "cells 中的格子格式不合法"
    return True, ""
