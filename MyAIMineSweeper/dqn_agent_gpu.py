# -*- coding: utf-8 -*-
"""GPU 版 DQN：结构与 dqn_agent.py 一致，新增 CUDA、混合精度(AMP)、网络宽度可调。

用法：配合 train_dqn_gpu.py 使用；评估时 evaluate.py --agent dqn_gpu。
"""

import random
from collections import deque

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F


class DQNNetGPU(nn.Module):
    """与 CPU 版同构的小 CNN，width 控制卷积通道数（GPU 上可加宽）。"""

    def __init__(self, rows, cols, channels=4, width=32):
        super().__init__()
        self.rows = rows
        self.cols = cols
        self.n_actions = rows * cols
        self.conv = nn.Sequential(
            nn.Conv2d(channels, width, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.Conv2d(width, width * 2, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.Conv2d(width * 2, width * 2, kernel_size=3, padding=1),
            nn.ReLU(),
        )
        self.head = nn.Sequential(
            nn.Flatten(),
            nn.Linear(width * 2 * rows * cols, 256),
            nn.ReLU(),
            nn.Linear(256, self.n_actions),
        )

    def forward(self, x):
        return self.head(self.conv(x))


class ReplayBufferGPU:
    """经验回放：与 CPU 版相同，数据在 optimize 时一次性搬到 GPU。"""

    def __init__(self, capacity):
        self.buf = deque(maxlen=capacity)

    def push(self, s, a, r, s2, d):
        self.buf.append((s, a, r, s2, d))

    def sample(self, batch_size):
        batch = random.sample(self.buf, batch_size)
        s = torch.from_numpy(np.stack([x[0] for x in batch])).float()
        a = torch.tensor([x[1] for x in batch], dtype=torch.long).unsqueeze(1)
        r = torch.tensor([x[2] for x in batch], dtype=torch.float32).unsqueeze(1)
        s2 = torch.from_numpy(np.stack([x[3] for x in batch])).float()
        d = torch.tensor([x[4] for x in batch], dtype=torch.float32).unsqueeze(1)
        return s, a, r, s2, d

    def __len__(self):
        return len(self.buf)


class DQNAgentGPU:
    """GPU 优先的 DQN 智能体；无 GPU 时自动退化为 CPU 并给出警告。"""

    def __init__(self, env, lr=1e-3, gamma=0.99,
                 epsilon_start=1.0, epsilon_end=0.05, epsilon_decay_steps=500000,
                 buffer_capacity=500000, batch_size=256, target_update_freq=2000,
                 net_width=32, amp=True, device=None):
        self.env = env
        self.rows, self.cols = env.rows, env.cols
        self.n_actions = self.rows * self.cols
        self.device = device or ("cuda" if torch.cuda.is_available() else "cpu")
        if self.device.startswith("cuda") and not torch.cuda.is_available():
            self.device = "cpu"
        if self.device == "cpu":
            print("[WARN] 未检测到 CUDA GPU，GPU 版将退化为 CPU 训练")
        self.amp = bool(amp) and self.device.startswith("cuda")

        self.policy_net = DQNNetGPU(self.rows, self.cols, width=net_width).to(self.device)
        self.target_net = DQNNetGPU(self.rows, self.cols, width=net_width).to(self.device)
        self.target_net.load_state_dict(self.policy_net.state_dict())
        self.target_net.eval()
        self.optimizer = torch.optim.Adam(self.policy_net.parameters(), lr=lr)
        self.scaler = torch.amp.GradScaler("cuda", enabled=self.amp)

        self.gamma = gamma
        self.epsilon_start = epsilon_start
        self.epsilon_end = epsilon_end
        self.epsilon_decay_steps = epsilon_decay_steps
        self.epsilon = epsilon_start
        self.steps = 0
        self.buffer = ReplayBufferGPU(buffer_capacity)
        self.batch_size = batch_size
        self.target_update_freq = target_update_freq
        self.net_width = net_width

    def decay_epsilon(self):
        frac = min(1.0, self.steps / max(self.epsilon_decay_steps, 1))
        self.epsilon = self.epsilon_start - frac * (self.epsilon_start - self.epsilon_end)

    def act(self, state, valid, eval_mode=False):
        """epsilon-greedy；eval_mode=True 时纯贪婪。返回合法动作索引。"""
        self.steps += 1
        if not eval_mode:
            self.decay_epsilon()
        if not eval_mode and random.random() < self.epsilon:
            return random.choice(valid)
        with torch.no_grad():
            q = self.policy_net(
                torch.from_numpy(state).unsqueeze(0).to(self.device))[0]
            q = q.cpu().numpy()
        mask = np.full(self.n_actions, -np.inf)
        for a in valid:
            mask[a] = q[a]
        return int(np.argmax(mask))

    def remember(self, s, a, r, s2, d):
        self.buffer.push(s, a, r, s2, d)

    def optimize(self):
        if len(self.buffer) < self.batch_size:
            return None
        s, a, r, s2, d = self.buffer.sample(self.batch_size)
        s, a, r, s2, d = (x.to(self.device) for x in (s, a, r, s2, d))
        with torch.autocast(device_type="cuda", enabled=self.amp):
            q = self.policy_net(s).gather(1, a)
            with torch.no_grad():
                q_next = self.target_net(s2).max(1, keepdim=True).values
                target = r + self.gamma * q_next * (1.0 - d)
            loss = F.smooth_l1_loss(q, target)
        self.optimizer.zero_grad()
        self.scaler.scale(loss).backward()
        self.scaler.step(self.optimizer)
        self.scaler.update()
        if self.steps % self.target_update_freq == 0:
            self.target_net.load_state_dict(self.policy_net.state_dict())
        return float(loss.item())

    def save(self, path):
        torch.save({
            "policy": self.policy_net.state_dict(),
            "rows": self.rows,
            "cols": self.cols,
            "steps": self.steps,
            "net_width": self.net_width,
        }, path)

    @classmethod
    def load(cls, path, env, **kwargs):
        ckpt = torch.load(path, map_location="cpu")
        kwargs.setdefault("net_width", ckpt.get("net_width", 32))
        agent = cls(env, **kwargs)
        agent.policy_net.load_state_dict(ckpt["policy"])
        agent.target_net.load_state_dict(ckpt["policy"])
        return agent
