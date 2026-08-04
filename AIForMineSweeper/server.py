#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""AIForMineSweeper 求解服务入口。

用法:
    python server.py                # 启动本地 HTTP 服务 (默认 127.0.0.1:8765)
    python server.py --dry-run      # 使用内置启发式策略（不调用 DeepSeek，用于联调）
    python server.py --test         # 用示例棋盘调用一次 DeepSeek 验证 API Key 与网络

逻辑均拆分在 app/ 包内，本文件只做入口转发。
"""

import sys

from app.cli import main


if __name__ == "__main__":
    sys.exit(main())
