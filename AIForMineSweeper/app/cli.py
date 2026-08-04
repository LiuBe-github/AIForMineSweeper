# -*- coding: utf-8 -*-
"""命令行入口：python server.py [--dry-run | --test]。"""

import argparse
import json
import sys

from .config import get_api_key, load_config
from .httpd import run_server, set_dry_run
from .solvers import solve_with_deepseek


def selftest():
    cfg = load_config()
    key = get_api_key(cfg)
    if not key:
        print("[FAIL] 未配置 API Key。请设置环境变量 %s 或在 config.json 填写 api_key。"
              % (cfg.get("api_key_env") or "DEEPSEEK_API_KEY"))
        return 1
    payload = {
        "rows": 5,
        "cols": 5,
        "mines": 4,
        "cells": [
            {"state": "revealed", "adjacent": 0}, {"state": "revealed", "adjacent": 0},
            {"state": "revealed", "adjacent": 1}, {"state": "hidden"}, {"state": "hidden"},
            {"state": "revealed", "adjacent": 0}, {"state": "revealed", "adjacent": 0},
            {"state": "revealed", "adjacent": 1}, {"state": "hidden"}, {"state": "hidden"},
            {"state": "revealed", "adjacent": 1}, {"state": "revealed", "adjacent": 1},
            {"state": "revealed", "adjacent": 1}, {"state": "hidden"}, {"state": "hidden"},
            {"state": "hidden"}, {"state": "hidden"}, {"state": "hidden"},
            {"state": "hidden"}, {"state": "hidden"},
            {"state": "hidden"}, {"state": "hidden"}, {"state": "hidden"},
            {"state": "hidden"}, {"state": "hidden"},
        ],
    }
    print("模型: %s, Base URL: %s" % (cfg.get("model"), cfg.get("base_url")))
    try:
        move = solve_with_deepseek(payload, cfg)
        print("[OK] DeepSeek 返回下一步:", json.dumps(move, ensure_ascii=False))
        return 0
    except Exception as e:
        print("[FAIL] 调用 DeepSeek 失败: %s" % e)
        return 1


def main():
    parser = argparse.ArgumentParser(description="AIForMineSweeper 求解服务")
    parser.add_argument("--dry-run", action="store_true",
                        help="使用内置启发式策略，不调用 DeepSeek")
    parser.add_argument("--test", action="store_true",
                        help="用示例棋盘调用一次 DeepSeek 验证配置")
    args = parser.parse_args()

    if args.test:
        sys.exit(selftest())

    cfg = load_config()
    set_dry_run(args.dry_run)
    run_server(cfg.get("host", "127.0.0.1"), int(cfg.get("port", 8765)))


if __name__ == "__main__":
    main()
