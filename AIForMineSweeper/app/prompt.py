# -*- coding: utf-8 -*-
"""棋盘状态 -> 文本网格 -> Prompt。"""


def cell_to_char(cell):
    state = cell.get("state", "hidden")
    if state == "flagged":
        return "F"
    if state == "revealed":
        return str(cell.get("adjacent", 0))
    return "."  # hidden


def build_prompt(payload):
    rows = int(payload["rows"])
    cols = int(payload["cols"])
    mines = int(payload.get("mines", 0))
    cells = payload["cells"]

    flags = sum(1 for c in cells if c.get("state") == "flagged")
    revealed = sum(1 for c in cells if c.get("state") == "revealed")

    grid_lines = []
    for r in range(rows):
        row_chars = [cell_to_char(cells[r * cols + c]) for c in range(cols)]
        grid_lines.append("Row %2d: %s" % (r, " ".join(row_chars)))
    grid = "\n".join(grid_lines)

    prompt = f"""You are playing Minesweeper and must decide the next move.

Board size: {rows} rows x {cols} cols, total mines: {mines}, flags placed: {flags}, cells revealed: {revealed}.

Grid legend:
- "." means the cell is hidden (unknown).
- "F" means the cell is flagged by the player (do not reveal it).
- A number (0-8) means the cell has been revealed, and that number is how many mines are in its 8 neighboring cells.

Current board (rows and columns are 0-indexed):
{grid}

Rules for choosing a move:
1. Use action "chord" on a revealed number cell ONLY when the number of adjacent flags equals that number; it opens all remaining safe neighbors at once and is the most efficient move.
2. Use action "flag" ONLY when you can mathematically prove the hidden cell is a mine (e.g. a number already has exactly that many unopened neighbors left). Never flag based on a guess: a wrong flag makes the game unwinnable.
3. If no guaranteed move exists, choose the hidden cell least likely to be a mine (e.g. far from high numbers, near empty areas) and reveal it. When unsure, always prefer "reveal" over "flag".
4. Never reveal a flagged or already-revealed cell. Never chord a hidden cell.

Respond with ONLY one valid JSON object, no markdown fences, no extra text:
{{"action": "reveal" | "flag" | "chord", "row": <int>, "col": <int>, "reason": "<one short sentence>"}}
"""
    feedback = payload.get("feedback", "")
    if feedback:
        prompt += f"\nNote: your previous move was rejected because: {feedback}\nPlease return a different, valid move.\n"
    return prompt
