# AIForMineSweeper

用 Python 构建的本地接口服务：接收扫雷游戏的棋盘状态，调用模型推理下一步，
返回给游戏自动执行，循环直到通关。通关用时由游戏写入本文件夹的 `solve_records.txt`。

## 两种模型引擎（游戏内下拉栏选择）

游戏主界面「AI 解局」旁有一个下拉栏，可选择：

| 选项 | engine 值 | 说明 |
| --- | --- | --- |
| API Key 模型（DeepSeek） | `api` | 调用 DeepSeek 官方接口，使用你配置的 API Key 与 `model` |
| 网页默认模型（内置求解器） | `heuristic` | 本地启发式算法，不调用外部接口、不需要 API Key |

> 说明：目前项目中没有独立的“网页模型”服务端，所以“网页默认模型”对应的是
> 内置求解器（即原来的 `--dry-run` 策略）。如果你后续接入了真正的网页模型
> 服务，只需在 `app/solvers.py` 的 `resolve_engine` 中增加一个 engine 分支。

## 配置 API Key

两种方式任选其一（环境变量优先）：

1. 设置环境变量：

```bat
set DEEPSEEK_API_KEY=sk-xxxxxxxx
```

2. 编辑 `config.json`，填写 `api_key` 字段。

可选配置项：

- `model`：`deepseek-chat`（默认，速度快）或 `deepseek-reasoner`（推理更强，更慢）
- `base_url`：DeepSeek 官方地址 `https://api.deepseek.com`
- `port`：本地服务端口（默认 `8765`，与游戏内约定一致，一般不用改）
- `default_engine`：`api`（默认）或 `heuristic`；客户端未传 `engine` 时使用

## 启动服务

```bat
python server.py
```

看到 `listening on http://127.0.0.1:8765` 即就绪。游戏点击「AI帮解」按钮后会自动连接；
若游戏未能自动启动服务，也可以先手动运行上面这条命令。

## 验证配置（推荐先跑一次）

```bat
python server.py --test
```

会用一张示例棋盘调用一次 DeepSeek，返回下一步的动作即说明 API Key 与网络正常。

## 联调模式（不需要 API Key）

```bat
python server.py --dry-run
```

此时 `/solve` 强制使用内置启发式策略返回下一步，方便在没有 API Key 的情况下
验证游戏与服务的完整联动流程。

## HTTP 接口

- `GET /health` → `{"status":"ok","dry_run":false}`
- `POST /solve`

  ```json
  {
    "engine": "api",
    "rows": 9,
    "cols": 9,
    "mines": 10,
    "flagged": 0,
    "revealed": 1,
    "first_click_done": true,
    "cells": [
      {"state": "revealed", "adjacent": 0},
      {"state": "hidden"},
      {"state": "flagged"},
      ...
    ]
  }
  ```

  `engine` 可选：`api`（DeepSeek，默认）或 `heuristic`（内置求解器）；
  不传时使用 `config.json` 的 `default_engine`，再缺省为 `api`。

  返回：

  ```json
  {"action": "reveal", "row": 3, "col": 5, "reason": "安全"}
  ```

  `action` 取值：`reveal`（翻开）、`flag`（插旗）、`chord`（数字格快速翻开周围）。

## 目录结构

```text
AIForMineSweeper/
├── server.py            # 入口（薄封装）
├── config.json          # 本地配置（API Key 等）
└── app/                 # 核心逻辑拆分
    ├── config.py        # 配置加载
    ├── prompt.py        # 棋盘 -> Prompt
    ├── api_client.py    # DeepSeek API 调用与返回解析
    ├── validation.py    # 棋盘/AI 结果校验
    ├── solvers.py       # engine 分发 + 内置启发式求解器
    ├── httpd.py         # HTTP 服务
    └── cli.py           # 命令行参数
```

## 通关记录

游戏 AI 通关后，会把记录追加写入 `solve_records.txt`：

```text
========================================
时间: 2026-08-03 15:04:33
难度: 初级
棋盘: 9x9，10 雷
通关用时: 47 秒
AI 步数: 21
尝试次数: 1
结果: 通关
========================================
```
