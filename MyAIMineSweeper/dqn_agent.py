# -*- coding: utf-8 -*-
"""DQN：小 CNN 拟合 Q 值，mask 非法动作，经验回放 + 目标网络。"""

import random
from collections import deque

import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F


class DQNNet(nn.Module):
    def __init__(self, rows, cols, channels=4):
        super().__init__()
        self.rows = rows
        self.cols = cols
        self.n_actions = rows * cols
        self.conv = nn.Sequential(
            nn.Conv2d(channels, 32, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 64, kernel_size=3, padding=1),
            nn.ReLU(),
            nn.Conv2d(64, 64, kernel_size=3, padding=1),
            nn.ReLU(),
        )
        self.head = nn.Sequential(
            nn.Flatten(),
            nn.Linear(64 * rows * cols, 256),
            nn.ReLU(),
            nn.Linear(256, self.n_actions),
        )

    def forward(self, x):
        return self.head(self.conv(x))


class ReplayBuffer:
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


class DQNAgent:
    def __init__(self, env, lr=1e-3, gamma=0.99,
                 epsilon_start=1.0, epsilon_end=0.05, epsilon_decay_steps=200000,
                 buffer_capacity=100000, batch_size=64, target_update_freq=1000,
                 device=None):
        self.env = env
        self.rows, self.cols = env.rows, env.cols
        self.n_actions = self.rows * self.cols
        self.device = device or ("cuda" if torch.cuda.is_available() else "cpu")
        self.policy_net = DQNNet(self.rows, self.cols).to(self.device)
        self.target_net = DQNNet(self.rows, self.cols).to(self.device)
        self.target_net.load_state_dict(self.policy_net.state_dict())
        self.target_net.eval()
        self.optimizer = torch.optim.Adam(self.policy_net.parameters(), lr=lr)
        self.gamma = gamma
        self.epsilon_start = epsilon_start
        self.epsilon_end = epsilon_end
        self.epsilon_decay_steps = epsilon_decay_steps
        self.epsilon = epsilon_start
        self.steps = 0
        self.buffer = ReplayBuffer(buffer_capacity)
        self.batch_size = batch_size
        self.target_update_freq = target_update_freq

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
        q = self.policy_net(s).gather(1, a)
        with torch.no_grad():
            q_next = self.target_net(s2).max(1, keepdim=True).values
            target = r + self.gamma * q_next * (1.0 - d)
        loss = F.smooth_l1_loss(q, target)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        if self.steps % self.target_update_freq == 0:
            self.target_net.load_state_dict(self.policy_net.state_dict())
        return float(loss.item())

    def save(self, path):
        torch.save({
            "policy": self.policy_net.state_dict(),
            "rows": self.rows,
            "cols": self.cols,
            "steps": self.steps,
        }, path)

    @classmethod
    def load(cls, path, env, **kwargs):
        ckpt = torch.load(path, map_location="cpu")
        agent = cls(env, **kwargs)
        agent.policy_net.load_state_dict(ckpt["policy"])
        agent.target_net.load_state_dict(ckpt["policy"])
        return agent
