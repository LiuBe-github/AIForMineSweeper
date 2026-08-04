# AIForMineSweeper

一个用 **AI 自动通关扫雷** 的项目：经典扫雷游戏（C++ / Qt 5）配合本地 Python 服务，
把棋盘状态交给求解引擎，引擎返回下一步动作，游戏自动执行并循环直到通关。

```text
MineSweeper.exe（Qt 客户端）
        │   本地 HTTP（127.0.0.1:8765）
        ▼
server.py（Python 本地服务）
        │
        ├─ engine=api        → DeepSeek API（仅“需要猜”的局面才调用）
        └─ engine=heuristic  → 内置确定性推理 + 概率估计（纯本地，无需联网）
```

## 功能特性

### 扫雷游戏（MineSweeper/）

- 三种预设难度：初级 9×9/10 雷、中级 16×16/40 雷、高级 30×16/99 雷，支持自定义棋盘
- 首次点击安全（并排除首格周围 3×3 区域布雷）
- 左键翻开、右键插旗、中键/双击数字格 chord 快速翻格、F2 重开
- 计时器、剩余雷数统计、经典 Windows 扫雷风格界面
- 「AI 解局」按钮 + 模型选择下拉栏，自动求解直到通关或手动停止

### AI 求解（AIForMineSweeper/）

- **双引擎**：API Key 模型（DeepSeek）与网页默认模型（内置求解器）
- **混合求解策略**：确定性推理（安全翻开、必定的雷插旗、旗数匹配 chord）在本地
  直接完成，只有真正需要“猜”的局面才调用 DeepSeek——既快又稳，且插旗完全正确
- **健壮性**：DeepSeek 瞬时错误自动重试；重试耗尽后回退本地求解，游戏不会因
  大模型输出异常而中断
- **可观测**：通关记录包含引擎、模型、网络延迟与去除网络等待后的真实用时

## 目录结构

```text
AIForMineSweeper/
├── MineSweeper/            # Qt 5 扫雷客户端
│   ├── MineSweeper.pro     # qmake 工程
│   ├── CMakeLists.txt      # CMake 工程
│   ├── build.bat           # 一键构建脚本
│   ├── run.bat             # 运行脚本
│   └── src/                # 客户端源码（MainWindow / MineField / CellButton …）
└── AIForMineSweeper/       # Python 本地求解服务
    ├── server.py           # 服务入口
    ├── config.example.json # 配置模板（可提交）
    ├── config.json         # 本地配置（含 API Key，不入库）
    └── app/                # 配置加载 / Prompt / API 调用 / 校验 / 求解器 / HTTP
```

## 环境要求

- Windows
- 客户端：Qt 5.15.2（本机为 Anaconda 内置 MSVC 版 Qt）+ Visual Studio（MSVC x64）
- 服务端：Python 3.9+（仅使用标准库，无需额外安装依赖）

## 构建客户端

在 `MineSweeper/` 目录下运行：

```bat
build.bat
```

脚本会调用 vcvarsall 初始化 MSVC 环境，用 qmake + nmake 编译，并自动部署 Qt 运行库。
产物为 `bin\MineSweeper.exe`。也可使用 qmake 或 CMake 手动构建，详见 `MineSweeper/README.md`。

## 运行

```bat
run.bat
```

或直接双击 `MineSweeper\bin\MineSweeper.exe`。

## 使用 AI 解局

游戏主界面点击「AI 解局」前，先在下拉栏选择引擎：

| 选项 | engine 值 | 说明 |
| --- | --- | --- |
| API Key 模型（DeepSeek） | `api` | 调用 DeepSeek 官方接口，需要配置 API Key 与模型 |
| 网页默认模型（内置求解器） | `heuristic` | 本地启发式算法，不联网、不需要 API Key |

### 1. 配置 API Key（仅 API 引擎需要）

两种方式任选其一，环境变量优先：

```bat
set DEEPSEEK_API_KEY=sk-xxxxxxxx
```

或编辑 `AIForMineSweeper/config.json` 填写 `api_key` 字段。常用配置项：

- `model`：`deepseek-chat`（默认，速度快）或 `deepseek-reasoner`（推理更强、更慢）
- `max_tokens`：默认 `8192`（使用 `deepseek-reasoner` 时必须足够大，否则思考会占满配额）
- `base_url`：DeepSeek 官方地址 `https://api.deepseek.com`
- `port`：本地服务端口，默认 `8765`，与游戏约定一致，一般不用改

> 注意：修改配置后必须**重启服务**，运行中的服务会继续使用旧配置。

### 2. 启动本地服务

```bat
python AIForMineSweeper\server.py
```

看到 `listening on http://127.0.0.1:8765` 即就绪。游戏找不到服务时也会尝试自动启动。

### 3. 验证配置（推荐）

```bat
python AIForMineSweeper\server.py --test
```

返回 `[OK] DeepSeek 返回下一步: {...}` 即说明 API Key 与网络正常。

### 4. 开始解局

回到游戏，选择引擎后点击「AI 解局」。游戏会逐格显示 AI 的动作，踩雷后自动重开一局
继续解，直到通关或手动点击「停止」。

## 联调模式（无 API Key）

```bat
python AIForMineSweeper\server.py --dry-run
```

此时 `/solve` 强制使用内置启发式策略，方便在没有 API Key 的情况下验证完整联动流程。

## HTTP 接口

### `GET /health`

```json
{"status": "ok", "dry_run": false}
```

### `POST /solve`

请求：

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
    {"state": "flagged"}
  ]
}
```

响应（`engine` / `model` 用于记录实际使用的 AI 信息）：

```json
{
  "action": "reveal",
  "row": 3,
  "col": 5,
  "reason": "chord: flags match number",
  "engine": "api",
  "model": "deepseek-chat"
}
```

`action` 取值：`reveal`（翻开）、`flag`（插旗）、`chord`（数字格快速翻开周围）。

## 通关记录

AI 通关后记录追加写入 `AIForMineSweeper/solve_records.txt`：

```text
========================================
时间: 2026-08-04 15:35:07
难度: 初级
棋盘: 9x9，10 雷
引擎: API Key 模型（DeepSeek）
模型: deepseek-chat
总用时(含网络等待): 6 秒
网络延迟合计: 1300 毫秒
通关用时(去网络延迟): 5 秒
AI 步数: 22
尝试次数: 1
结果: 通关
========================================
```

- 「网络延迟合计」为每次 `/solve` 请求从发出到收到的客户端往返耗时之和
- 「通关用时(去网络延迟)」= 总用时 − 网络延迟合计，反映调用网络 AI 的真实本地耗时
- 使用内置求解器时引擎记录为“网页默认模型（内置求解器）”，延迟合计为 0

## 常见问题

- **提示“无法连接 AI 服务”**：本地服务未启动，手动运行 `python AIForMineSweeper\server.py`。
- **提示“AI 服务返回错误”**：游戏会直接显示服务端真实原因，常见为 API Key 无效/
  余额不足、网络不通、模型输出异常。服务端已自动重试并在重试耗尽后回退本地求解。
- **使用 `deepseek-reasoner` 返回空内容**：思考占满了 `max_tokens`，请将
  `config.json` 的 `max_tokens` 调大到 8192 以上并重启服务。
- **改了配置没生效**：重启服务，运行中的进程会保留启动时的配置。
- **端口 8765 被占用 / 多个服务抢请求**：新版服务已禁止同一端口双绑定；如仍异常，
  检查任务管理器是否有残留的 python 进程并结束。
- **通关率与速度**：混合策略下 9x9 初级实测 9~19 秒通关；追求极致效率可选用
  “网页默认模型（内置求解器）”（纯本地、零费用）。

## 安全说明

`config.json` 含真实 API Key，已被 `.gitignore` 排除、不随代码提交；仓库只包含
`config.example.json` 模板。新环境请复制模板再填写自己的 Key。
