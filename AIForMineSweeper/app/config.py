# -*- coding: utf-8 -*-
"""配置加载：config.json + 环境变量。"""

import json
import os

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONFIG_PATH = os.path.join(BASE_DIR, "config.json")

DEFAULT_CONFIG = {
    "api_key": "",             # 也可以直接填在这里；优先读环境变量
    "api_key_env": "DEEPSEEK_API_KEY",
    "base_url": "https://api.deepseek.com",
    "model": "deepseek-chat",  # 可换成 deepseek-reasoner
    "temperature": 0.0,
    "max_tokens": 1024,
    "host": "127.0.0.1",
    "port": 8765,
    "max_retries": 3,
    # 默认引擎：api = DeepSeek(用 API Key)；heuristic = 内置求解器(网页默认/免 Key)
    "default_engine": "api",
}


def load_config():
    cfg = dict(DEFAULT_CONFIG)
    if os.path.exists(CONFIG_PATH):
        try:
            with open(CONFIG_PATH, "r", encoding="utf-8") as f:
                cfg.update(json.load(f))
        except Exception as e:
            print("[WARN] 读取 config.json 失败，使用默认配置: %s" % e)
    return cfg


def get_api_key(cfg):
    env_name = cfg.get("api_key_env") or "DEEPSEEK_API_KEY"
    key = os.environ.get(env_name, "").strip()
    if not key:
        key = str(cfg.get("api_key", "")).strip()
    return key
