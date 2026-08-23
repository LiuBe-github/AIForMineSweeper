<div align="center">

# AIForMineSweeper

> *"让 AI 替你把雷区走完 —— 经典扫雷，自动通关。"*

[![Windows](https://img.shields.io/badge/Windows-10%20%2F%2011-0078D6)](https://www.microsoft.com/windows)
[![C++ / Qt 5](https://img.shields.io/badge/C%2B%2B-Qt%205.15-red)](https://www.qt.io)
[![Python](https://img.shields.io/badge/Python-3.9%2B-3776AB)](https://www.python.org)
[![DeepSeek](https://img.shields.io/badge/AI-DeepSeek-4D6BFE)](https://platform.deepseek.com)
[![Reinforcement Learning](https://img.shields.io/badge/RL-DQN%20%2F%20CSP-FF6F00)]()
[![GitHub Repo](https://img.shields.io/badge/GitHub-LiuBe--github%2FAIForMineSweeper-181717)](https://github.com/LiuBe-github/AIForMineSweeper)

<br>

想看 AI 到底怎么解扫雷？<br>
不想手动画棋盘、又想验证求解算法对不对？<br>
想自己从零训练一个本地扫雷 AI 模型？<br>

**一个用 AI 自动通关经典扫雷的项目：Qt 5 客户端 + Python 本地求解服务，支持「大模型 API + 纯本地启发式」双引擎；另含一个离线强化学习训练项目，可训练你自己的扫雷 AI。**

<br>

[功能特性](#-功能特性) · [系统架构](#-系统架构) · [快速开始](#快速开始) · [使用 AI 解局](#使用-ai-解局) · [本地 RL 训练](#本地强化学习训练-myaiminesweeper) · [技术架构](#-技术架构) · [常见问题](#常见问题)

</div>

---

> 🆕 **MyAIMineSweeper 上线（2026-08-08）** — 新增**本地强化学习训练项目**：9×9 扫雷环境（规则与游戏完全一致）+ CSP 约束求解 baseline（通关率约 72.8%）+ DQN 训练（CPU / GPU 双版本，GPU 支持 CUDA + 混合精度 AMP），并附 `random` / `csp` / `dqn` / `dqn_gpu` 一键评估对比。训练产物规划接入服务端 `engine=local`，成为新的本地求解引擎。

> 🆕 **AI 求解完整实现（2026-08-04）** — Qt 客户端 + Python 本地服务打通；双引擎（DeepSeek API / 内置启发式）混合求解，仅「需要猜」的局面才调用大模型；移除自定义棋盘上限并大幅优化大棋盘性能。

---

## 🧩 功能特性

| 模块 | 说明 |
|------|------|
| 🎮 **扫雷游戏** | 经典 Windows 风格界面（C++ / Qt 5），初级 9×9/10 雷、中级 16×16/40 雷、高级 30×16/99 雷，支持自定义棋盘；左键翻开、右键插旗、中键/双击 chord 快速翻格、F2 重开；首次点击安全（并排除首格周围 3×3 布雷） |
| 🤖 **AI 求解（双引擎）** | **DeepSeek API 引擎**（大模型，仅「需要猜」的局面才调用）+ **内置启发式引擎**（确定性推理 + 概率估计，纯本地、零费用）；混合策略把「必然解」留在本地完成，又快又稳，插旗完全正确 |
| 🧠 **本地强化学习训练** | 离线 RL 项目 MyAIMineSweeper：9×9 扫雷环境 + CSP baseline + DQN（CPU/GPU），可训练自己的模型，再接入服务端成为新的求解引擎 |
| 🛡 **健壮性** | DeepSeek 瞬时错误自动重试；重试耗尽后回退本地求解，游戏不因大模型异常而中断 |
| 📊 **可观测** | 通关记录含引擎、模型、网络延迟与「去除网络等待后的真实用时」，方便对比两种引擎效率 |

**贯穿全局**：客户端与服务端本地 HTTP 通信（127.0.0.1:8765）· API Key 仅存本地、不入库 · 踩雷自动重开续解 · 配置修改后重启服务即生效。

---

## 🖧 系统架构

```text
MineSweeper.exe（Qt 客户端，Windows）
        │   本地 HTTP（127.0.0.1:8765）
        ▼
server.py（Python 本地服务，跨平台）
        │
        ├─ engine=api        → DeepSeek API（仅“需要猜”的局面才调用）
        ├─ engine=heuristic  → 内置确定性推理 + 概率估计（纯本地，无需联网）
        └─ engine=local（规划中）→ 加载 MyAIMineSweeper 训练出的本地模型

MyAIMineSweeper（离线 RL 训练，先做 9×9）
        └─ 训练 DQN（CPU/GPU）→ 导出模型 → 接入服务端 local 引擎
```

整体是一个「**游戏客户端 → 本地求解服务 → 求解引擎**」的闭环：游戏把棋盘状态发给服务，服务选择引擎算出下一步动作并返回，游戏执行后再次上报，循环直到通关或手动停止。

---

## 快速开始

### 1. 构建客户端

在 `MineSweeper/` 目录下运行（需要 **Visual Studio / MSVC x64** 与 **Qt 5.15.2**）：

```bat
build.bat
```

脚本会调用 `vcvarsall` 初始化 MSVC 环境，用 qmake + nmake 编译并自动部署 Qt 运行库，产物为 `MineSweeper\bin\MineSweeper.exe`。也可使用 qmake 或 CMake 手动构建，详见 `MineSweeper/README.md`。

### 2. 启动本地服务

需要 **Python 3.9+**（仅用标准库，无需额外安装依赖）：

```bat
python AIForMineSweeper\server.py
```

看到 `listening on http://127.0.0.1:8765` 即就绪。游戏找不到服务时也会尝试自动启动。

### 3. 开始解局

直接双击 `MineSweeper\bin\MineSweeper.exe` 运行游戏，选择引擎后点击「AI 解局」即可。详见下方[使用 AI 解局](#使用-ai-解局)。

> 没有 API Key 也能跑通完整联动：见[联调模式](#联调模式无-api-key)。

---

## 🖱 使用说明

| 操作 | 效果 |
|------|------|
| 左键点击 | 翻开格子 |
| 右键点击 | 在格子上插旗 / 取消旗 |
| 中键 / 双击数字格 | chord：若周围旗数等于数字，快速翻开其余格 |
| `F2` | 重开一局 |
| 下拉栏选引擎 | 切换 `api`（DeepSeek）或 `heuristic`（内置求解器） |
| 「AI 解局」按钮 | 启动自动求解，逐格显示 AI 动作，踩雷后自动重开续解 |
| 「停止」按钮 | 随时中断自动求解 |

### 难度与棋盘

| 难度 | 棋盘 | 雷数 |
|------|------|------|
| 初级 | 9×9 | 10 |
| 中级 | 16×16 | 40 |
| 高级 | 30×16 | 99 |
| 自定义 | 任意 | 任意 |

---

## 🤖 使用 AI 解局

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

回到游戏，选择引擎后点击「AI 解局」。游戏会逐格显示 AI 的动作，踩雷后自动重开一局继续解，直到通关或手动点击「停止」。

---

## 联调模式（无 API Key）

```bat
python AIForMineSweeper\server.py --dry-run
```

此时 `/solve` 强制使用内置启发式策略，方便在没有 API Key 的情况下验证完整联动流程。

---

## 本地强化学习训练（MyAIMineSweeper）

项目目标是训练一个本地模型解 9×9 扫雷，并追求解题效率（通关率、步数、单步延迟）。详见 `MyAIMineSweeper/README.md`。

### 安装依赖

```bat
cd MyAIMineSweeper
python -m venv .venv
.venv\Scripts\python -m pip install -r requirements.txt
```

有 NVIDIA GPU（如 RTX 3060）时，另建 GPU 环境安装 CUDA 版 PyTorch：

```bat
python -m venv .venv-gpu
.venv-gpu\Scripts\python -m pip install numpy
.venv-gpu\Scripts\python -m pip install torch --index-url https://download.pytorch.org/whl/cu128
```

### 训练

CPU：

```bat
.venv\Scripts\python train_dqn.py --episodes 20000
```

GPU（RTX 3060，默认开启混合精度）：

```bat
.venv-gpu\Scripts\python train_dqn_gpu.py --episodes 100000 --amp --net-width 64
```

每 `--log-every` 局打印一次滚动通关率，模型自动保存到 `checkpoints/`。

### 评估

```bat
.venv\Scripts\python evaluate.py --agent csp --episodes 500        # 基准线
.venv\Scripts\python evaluate.py --agent dqn --model checkpoints\dqn_9x9.pt
.venv-gpu\Scripts\python evaluate.py --agent dqn_gpu --model checkpoints\dqn_gpu_9x9.pt
```

当前 9×9 基准（供模型效果对比）：

| 求解器 | 通关率 | 平均步数 | 单步延迟 |
| --- | --- | --- | --- |
| 随机 | ~0% | 4~5 | 0.1ms |
| CSP 约束求解 | ~72.8% | ~24.8 | 0.1ms |

### 后续规划

- 给 DQN 增加插旗 / chord 动作，进一步提高胜率与步数效率
- 约束概率 + NN 混合：NN 只在无必然解时做猜测
- 把训练好的模型接入服务端，新增 `engine=local`，游戏下拉栏增加“本地模型”选项

---

## ⚙ 技术架构

| 模块 | 实现 |
|------|------|
| 扫雷客户端 | C++ / Qt 5，`MainWindow` + `MineField` + `BoardWidget`；左键翻开 / 右键插旗 / 中键 chord、F2 重开；计时器、剩余雷数、经典风格界面；「AI 解局」按钮 + 引擎下拉栏 |
| 本地服务 | Python 3.9+ 标准库（`http.server`），监听 `127.0.0.1:8765`；解析棋盘 → 调引擎 → 返回动作；单端口禁止双绑定 |
| API 引擎 | DeepSeek Chat Completions；仅「需要猜」的局面才调用；瞬时错误自动重试，耗尽后回退本地求解 |
| 启发式引擎 | 确定性推理（安全翻开、必雷插旗、旗数匹配 chord）+ 概率估计，纯本地零费用 |
| 混合策略 | 本地先行解决必然解，无必然解才上大模型，兼顾速度与正确率、插旗完全正确 |
| 本地模型引擎 | 规划中：加载 MyAIMineSweeper 训练的 DQN 模型（`engine=local`） |
| RL 训练 | 9×9 环境（规则与游戏一致，首击安全、洪泛展开、插旗、胜负判定）；CSP baseline ~72.8%；DQN CPU/GPU（CUDA + 混合精度 AMP），网络同构、结果可互评 |
| 配置与安全 | `config.json`（含 API Key）已被 `.gitignore` 排除、不入库；仓库仅含 `config.example.json` 模板 |

> 设计取舍：把「必然解」留在本地确定性推理，只有真正需要博弈才调用大模型——既避免无谓的 API 开销与延迟，也保证插旗等操作的 100% 正确率；大模型异常时回退本地，游戏永不中断。

---

## 📁 目录结构

```text
AIForMineSweeper/
├── MineSweeper/            # Qt 5 扫雷客户端（Windows / MSVC）
│   ├── MineSweeper.pro     # qmake 工程
│   ├── CMakeLists.txt      # CMake 工程
│   ├── build.bat           # 一键构建脚本
│   ├── run.bat             # 运行脚本
│   └── src/                # 客户端源码（MainWindow / MineField / BoardWidget …）
├── AIForMineSweeper/       # Python 本地求解服务
│   ├── server.py           # 服务入口
│   ├── config.example.json # 配置模板（可提交）
│   ├── config.json         # 本地配置（含 API Key，不入库）
│   └── app/                # 配置加载 / Prompt / API 调用 / 校验 / 求解器 / HTTP
└── MyAIMineSweeper/        # 本地强化学习训练项目（9×9 DQN）
    ├── minesweeper_env.py  # 扫雷环境（规则与游戏一致）
    ├── csp_solver.py       # 约束求解 baseline
    ├── dqn_agent.py        # CPU 版 DQN
    ├── dqn_agent_gpu.py    # GPU 版 DQN（CUDA + AMP）
    ├── train_dqn.py        # CPU 训练入口
    ├── train_dqn_gpu.py    # GPU 训练入口
    └── evaluate.py         # random / csp / dqn / dqn_gpu 评估
```

---

## 环境要求

- **客户端**：Windows 10 / 11，Qt 5.15.2（MSVC 版）+ Visual Studio（MSVC x64）
- **服务端**：Python 3.9+（仅标准库，无需额外依赖）
- **RL 训练**：Python 3.9+，`numpy` + `torch`（CPU 版即可；有 NVIDIA GPU 可装 CUDA 版）

---

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

---

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

---

## 常见问题

- **提示“无法连接 AI 服务”**：本地服务未启动，手动运行 `python AIForMineSweeper\server.py`。
- **提示“AI 服务返回错误”**：游戏会直接显示服务端真实原因，常见为 API Key 无效 / 余额不足、网络不通、模型输出异常。服务端已自动重试并在重试耗尽后回退本地求解。
- **使用 `deepseek-reasoner` 返回空内容**：思考占满了 `max_tokens`，请将 `config.json` 的 `max_tokens` 调大到 8192 以上并重启服务。
- **改了配置没生效**：重启服务，运行中的进程会保留启动时的配置。
- **端口 8765 被占用 / 多个服务抢请求**：新版服务已禁止同一端口双绑定；如仍异常，检查任务管理器是否有残留的 python 进程并结束。
- **通关率与速度**：混合策略下 9×9 初级实测 9~19 秒通关；追求极致效率可选用“网页默认模型（内置求解器）”（纯本地、零费用）。
- **RL 训练通关率一直为 0**：训练早期 ε 还很大、模型在纯探索，属正常现象；跑满探索衰减（默认 20 万~50 万步）后再看滚动通关率，并优先与 CSP 基准对比。

---

## 安全说明

`config.json` 含真实 API Key，已被 `.gitignore` 排除、不随代码提交；仓库只包含 `config.example.json` 模板。新环境请复制模板再填写自己的 Key。

---

## 📜 更新日志

- **2026-08-08**: 新增 MyAIMineSweeper 本地强化学习训练项目（9×9 扫雷 DQN，CPU/GPU 双版本，GPU 支持 CUDA + 混合精度 AMP）；重写项目根 README 补充 RL 训练说明
- **2026-08-04**: AI 求解完整实现（Qt 客户端 + Python 本地服务打通，双引擎混合求解）；移除自定义棋盘上限并大幅优化大棋盘性能；用完整项目 README 替换占位 README
- **2026-08-04**: 初始完整提交（Initial commit），确立「游戏客户端 → 本地求解服务 → 求解引擎」整体架构

---

<div align="center">

Made with 💣 by [LiuBe](https://github.com/LiuBe-github) · Qt 5 / Python / DeepSeek / DQN

</div>
