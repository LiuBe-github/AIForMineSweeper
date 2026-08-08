# -*- coding: utf-8 -*-
"""DQN 训练入口。

用法:
    python train_dqn.py --episodes 20000
"""

import argparse
import os
import time

from dqn_agent import DQNAgent
from minesweeper_env import MinesweeperEnv


def main():
    ap = argparse.ArgumentParser(description="训练 DQN 解 9x9 扫雷")
    ap.add_argument("--rows", type=int, default=9)
    ap.add_argument("--cols", type=int, default=9)
    ap.add_argument("--mines", type=int, default=10)
    ap.add_argument("--episodes", type=int, default=20000)
    ap.add_argument("--batch-size", type=int, default=64)
    ap.add_argument("--lr", type=float, default=1e-3)
    ap.add_argument("--gamma", type=float, default=0.99)
    ap.add_argument("--epsilon-start", type=float, default=1.0)
    ap.add_argument("--epsilon-end", type=float, default=0.05)
    ap.add_argument("--epsilon-decay", type=int, default=200000)
    ap.add_argument("--replay-capacity", type=int, default=100000)
    ap.add_argument("--target-update", type=int, default=1000)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--save", default="checkpoints/dqn_9x9.pt")
    ap.add_argument("--log-every", type=int, default=100)
    args = ap.parse_args()

    save_dir = os.path.dirname(args.save)
    if save_dir:
        os.makedirs(save_dir, exist_ok=True)

    env = MinesweeperEnv(args.rows, args.cols, args.mines, seed=args.seed)
    agent = DQNAgent(env, lr=args.lr, gamma=args.gamma,
                     epsilon_start=args.epsilon_start,
                     epsilon_end=args.epsilon_end,
                     epsilon_decay_steps=args.epsilon_decay,
                     buffer_capacity=args.replay_capacity,
                     batch_size=args.batch_size,
                     target_update_freq=args.target_update)

    wins = 0
    total_steps = 0
    ep_loss_sum = 0.0
    ep_reward_sum = 0.0
    t0 = time.time()

    for ep in range(1, args.episodes + 1):
        state = env.reset()
        done = False
        ep_steps = 0
        ep_reward = 0.0
        ep_loss = 0.0
        while not done:
            valid = env.valid_actions()
            action = agent.act(state, valid)
            next_state, reward, done, info = env.step(action)
            agent.remember(state, action, reward, next_state, done)
            loss = agent.optimize()
            if loss is not None:
                ep_loss += loss
            state = next_state
            ep_reward += reward
            ep_steps += 1
            if ep_steps > args.rows * args.cols * 4:  # 防死循环
                break
        total_steps += ep_steps
        ep_reward_sum += ep_reward
        ep_loss_sum += ep_loss
        if info.get("won"):
            wins += 1

        if ep % args.log_every == 0:
            print("[ep %6d] win_rate=%.3f avg_steps=%.1f eps=%.3f "
                  "avg_loss=%.4f avg_reward=%.2f elapsed=%.1fs"
                  % (ep, wins / args.log_every, total_steps / args.log_every,
                     agent.epsilon, ep_loss_sum / args.log_every,
                     ep_reward_sum / args.log_every, time.time() - t0))
            wins = 0
            total_steps = 0
            ep_loss_sum = 0.0
            ep_reward_sum = 0.0
            t0 = time.time()
            agent.save(args.save)

    agent.save(args.save)
    print("训练完成，模型已保存到", args.save)


if __name__ == "__main__":
    main()
