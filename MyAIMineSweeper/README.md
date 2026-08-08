# MyAIMineSweeper

在本目录用**强化学习**训练一个本地 AI 解扫雷（先做 9x9 / 10 雷），追求解题效率
（通关率、步数、单步延迟）。

## 项目结构

```text
MyAIMineSweeper/
├── minesweeper_env.py   # 扫雷环境（规则与 C++ 版 MineField 对齐）
├── csp_solver.py        # 约束求解 baseline（用于对比，也是给 NN 的“老师”）
├── dqn_agent.py         # DQN：小 CNN + 经验回放 + 目标网络
├── train_dqn.py         # 训练入口
├── evaluate.py          # 评估：random / csp / dqn 对比
├── requirements.txt
└── checkpoints/         # 训练产物
```

## 安装依赖（CPU 版即可）

```bat
python -m venv .venv
.venv\Scripts\python -m pip install --upgrade pip
.venv\Scripts\python -m pip install -r requirements.txt
```

如果默认源太慢，可加 `-i https://pypi.tuna.tsinghua.edu.cn/simple`。

## GPU 训练（RTX 3060）

本机有 NVIDIA GPU（RTX 3060），建议单独建一个 GPU 虚拟环境，安装 CUDA 版 PyTorch：

```bat
python -m venv .venv-gpu
.venv-gpu\Scripts\python -m pip install --upgrade pip
.venv-gpu\Scripts\python -m pip install numpy
.venv-gpu\Scripts\python -m pip install torch --index-url https://download.pytorch.org/whl/cu128
```

训练（GPU 版模型，默认开启混合精度）：

```bat
.venv-gpu\Scripts\python train_dqn_gpu.py --episodes 100000 --amp
```

常用参数：`--net-width 64` 可加宽网络（GPU 富余算力换容量）、
`--epsilon-decay` 控制探索、`--device cuda:0` 指定设备。

评估 GPU 版模型（CPU/GPU 环境均可跑，自动加载到 CPU 推理）：

```bat
.venv-gpu\Scripts\python evaluate.py --agent dqn_gpu --model checkpoints\dqn_gpu_9x9.pt --episodes 1000
```

> CPU 版和 GPU 版只是"训练设备/速度"不同，网络结构和状态/动作/奖励定义完全一致，
> 训练结果可以直接互相评估对比。

## 训练

```bat
.venv\Scripts\python train_dqn.py --episodes 20000
```

每 `--log-every` 局打印一次滚动通关率，模型自动保存到 `checkpoints/dqn_9x9.pt`。

常用参数：

- `--episodes`：训练局数（先跑 20000 看趋势，再加大）
- `--epsilon-decay`：探索衰减步数（默认 200000，训练太慢可调小）
- `--lr`、`--batch-size`、`--gamma`：DQN 超参
- `--seed`：固定随机种子复现

## 评估

```bat
.venv\Scripts\python evaluate.py --agent random --episodes 500
.venv\Scripts\python evaluate.py --agent csp --episodes 500
.venv\Scripts\python evaluate.py --agent dqn --model checkpoints\dqn_9x9.pt --episodes 500
```

输出通关率、平均步数、平均单步延迟。**先跑 csp 拿到基准线**，之后所有模型效果
都跟它对比——这是"追求效率"的量化依据。

## 当前设计（第一版）

- 状态：4 通道棋盘张量 `(4, H, W)`：已翻开数字/8、旗子、隐藏格、剩余雷数比例
- 动作：`0..80` 共 81 个"翻开"动作；插旗/chord 暂未加入（后续扩展可减少步数）
- 奖励：安全推进 +0.05+0.02×新翻开格数；踩雷 -1；通关 +10；无效动作 -0.5
- 首击安全、0 格洪泛展开、通关判定与 C++ 版一致

已知简化（后续迭代点）：

1. DQN 的 target 计算未对下一状态的非法动作做 mask（对 9x9 影响不大）
2. 未加入插旗动作，也无法 chord，步数效率有上限
3. 奖励和探索曲线需要调参才能稳定收敛

## 下一步

1. 训练到通关率稳定后，把 NN 接到服务端：在
   `MineSweeperAPIVersion/AIForMineSweeper/app/solvers.py` 增加 `local` engine，
   游戏下拉栏加"本地模型"选项
2. 增加 flag/chord 动作，让智能体学会"确定性推理"，提高胜率和步数效率
3. 进阶：PPO / 约束概率 + NN 混合（NN 只在无必然解时做猜测）
