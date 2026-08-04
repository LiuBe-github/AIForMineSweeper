# -*- coding: utf-8 -*-
"""DeepSeek API 调用与 AI 输出解析。"""

import json
import urllib.error
import urllib.request

from .config import get_api_key
from .prompt import build_prompt


def call_deepseek(cfg, payload):
    key = get_api_key(cfg)
    if not key:
        raise RuntimeError(
            "缺少 DeepSeek API Key。请设置环境变量 %s，或在 config.json 中填写 api_key 字段。"
            % (cfg.get("api_key_env") or "DEEPSEEK_API_KEY")
        )

    url = cfg["base_url"].rstrip("/") + "/chat/completions"
    body = {
        "model": cfg.get("model", "deepseek-chat"),
        "messages": [
            {"role": "system", "content": "You are a careful Minesweeper solver. Always answer with valid JSON only."},
            {"role": "user", "content": build_prompt(payload)},
        ],
        "temperature": cfg.get("temperature", 0.0),
        "max_tokens": cfg.get("max_tokens", 1024),
        "response_format": {"type": "json_object"},
        "stream": False,
    }
    req = urllib.request.Request(
        url,
        data=json.dumps(body, ensure_ascii=False).encode("utf-8"),
        headers={
            "Content-Type": "application/json",
            "Authorization": "Bearer " + key,
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=180) as resp:
            data = json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        detail = ""
        try:
            detail = e.read().decode("utf-8", "replace")
        except Exception:
            pass
        raise RuntimeError("DeepSeek API HTTP %d: %s" % (e.code, detail))
    except urllib.error.URLError as e:
        raise RuntimeError("无法连接 DeepSeek API: %s" % e.reason)

    try:
        content = data["choices"][0]["message"]["content"].strip()
    except (KeyError, IndexError, TypeError) as e:
        raise RuntimeError("DeepSeek API 返回格式异常: %s" % data)
    return content


def parse_move(content):
    text = content.strip()
    if not text:
        raise ValueError(
            "AI 返回内容为空（常见原因：deepseek-reasoner 思考占满了 max_tokens，"
            "或模型未输出）。请调大 config.json 中的 max_tokens 后重启服务。"
        )
    # 去掉可能的 markdown 代码围栏
    if text.startswith("```"):
        first_nl = text.find("\n")
        last = text.rfind("```")
        text = text[first_nl + 1:last] if first_nl != -1 and last != -1 else text
    start = text.find("{")
    end = text.rfind("}")
    if start == -1 or end == -1 or end <= start:
        raise ValueError("AI 输出中找不到 JSON 对象: " + content[:200])
    obj = json.loads(text[start:end + 1])
    action = str(obj.get("action", "")).strip().lower()
    if action not in ("reveal", "flag", "chord"):
        raise ValueError("未知 action: %r" % action)
    return {
        "action": action,
        "row": int(obj["row"]),
        "col": int(obj["col"]),
        "reason": str(obj.get("reason", "")),
    }
