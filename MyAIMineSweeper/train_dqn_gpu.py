# -*- coding: utf-8 -*-
"""GPU 版 DQN 训练入口（结构同 train_dqn.py，默认参数面向 GPU）。

用法:
    python train_dqn_gpu.py --episodes 100000 --amp
"""

import argparse
import os
import time

from dqn_agent_gpu import DQNAgentGPU
from minesweeper_env import MinesweeperEnv


def main():
    ap = argparse.ArgumentParser(description="GPU 训练 DQN 解 9x9 扫雷")
    ap.add_argument("--rows", type=int, default=9)
    ap.add_argument("--cols", type=int, default=9)
    ap.add_argument("--mines", type=int, default=10)
    ap.add_argument("--episodes", type=int, default=50000)
    ap.add_argument("--batch-size", type=int, default=256)
    ap.add_argument("--lr", type=float, default=1e-3)
    ap.add_argument("--gamma", type=float, default=0.99)
    ap.add_argument("--epsilon-start", type=float, default=1.0)
    ap.add_argument("--epsilon-end", type=float, default=0.05)
    ap.add_argument("--epsilon-decay", type=int, default=500000)
    ap.add_argument("--replay-capacity", type=int, default=500000)
    ap.add_argument("--target-update", type=int, default=2000)
    ap.add_argument("--net-width", type=int, default=32,
                    help="卷积通道宽度，GPU 上可加大（如 64/128）")
    ap.add_argument("--amp", dest="amp", action="store_true",
                    help="启用混合精度（CUDA 下默认开启）")
    ap.add_argument("--no-amp", dest="amp", action="store_false")
    ap.set_defaults(amp=True)
    ap.add_argument("--device", default=None,
                    help="如 cuda:0；缺省自动检测")
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--save", default="checkpoints/dqn_gpu_9x9.pt")
    ap.add_argument("--log-every", type=int, default=1000)
    args = ap.parse_args()

    save_dir = os.path.dirname(args.save)
    if save_dir:
        os.makedirs(save_dir, exist_ok=True)

    env = MinesweeperEnv(args.rows, args.cols, args.mines, seed=args.seed)
    agent = DQNAgentGPU(env, lr=args.lr, gamma=args.gamma,
                        epsilon_start=args.epsilon_start,
                        epsilon_end=args.epsilon_end,
                        epsilon_decay_steps=args.epsilon_decay,
                        buffer_capacity=args.replay_capacity,
                        batch_size=args.batch_size,
                        target_update_freq=args.target_update,
                        net_width=args.net_width, amp=args.amp,
                        device=args.device)
    print("device=%s amp=%s net_width=%d" % (agent.device, agent.amp,
                                             agent.net_width))

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
