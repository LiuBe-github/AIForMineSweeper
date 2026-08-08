# -*- coding: utf-8 -*-
"""评估入口：random / csp / dqn，输出通关率、平均步数、平均单步延迟。

用法:
    python evaluate.py --agent random --episodes 500
    python evaluate.py --agent csp --episodes 500
    python evaluate.py --agent dqn --model checkpoints/dqn_9x9.pt --episodes 500
"""

import argparse
import time

from csp_solver import solve_step
from dqn_agent import DQNAgent
from dqn_agent_gpu import DQNAgentGPU
from minesweeper_env import MinesweeperEnv


def run_random(env, max_steps=400):
    steps = 0
    while True:
        valid = env.valid_actions()
        if not valid:
            break
        _, _, done, info = env.step(int(env.rng.choice(valid)))
        steps += 1
        if done or steps >= max_steps:
            return bool(info.get("won")), steps
    return False, steps


def run_csp(env, max_steps=400):
    steps = 0
    while True:
        move = solve_step(env)
        if move is None:
            break
        _, _, done, info = env.step(move)
        steps += 1
        if done or steps >= max_steps:
            return bool(info.get("won")), steps
    return False, steps


def run_dqn(env, agent, max_steps=400):
    state = env.reset()
    steps = 0
    while True:
        valid = env.valid_actions()
        if not valid:
            break
        action = agent.act(state, valid, eval_mode=True)
        state, _, done, info = env.step(action)
        steps += 1
        if done or steps >= max_steps:
            return bool(info.get("won")), steps
    return False, steps


def main():
    ap = argparse.ArgumentParser(description="评估扫雷求解器")
    ap.add_argument("--agent", choices=["random", "csp", "dqn", "dqn_gpu"],
                    default="csp")
    ap.add_argument("--rows", type=int, default=9)
    ap.add_argument("--cols", type=int, default=9)
    ap.add_argument("--mines", type=int, default=10)
    ap.add_argument("--episodes", type=int, default=500)
    ap.add_argument("--model", default="checkpoints/dqn_9x9.pt")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    env = MinesweeperEnv(args.rows, args.cols, args.mines, seed=args.seed)
    agent = None
    if args.agent == "dqn":
        agent = DQNAgent.load(args.model, env)
    elif args.agent == "dqn_gpu":
        agent = DQNAgentGPU.load(args.model, env)

    wins = 0
    total_steps = 0
    total_time = 0.0
    for _ in range(args.episodes):
        env.reset()
        t = time.time()
        if args.agent == "random":
            won, steps = run_random(env)
        elif args.agent == "csp":
            won, steps = run_csp(env)
        else:
            won, steps = run_dqn(env, agent)
        total_time += time.time() - t
        wins += int(won)
        total_steps += steps

    print("agent=%-6s episodes=%d win_rate=%.3f (%d/%d) "
          "avg_steps=%.1f avg_time_per_step=%.1fms total=%.1fs"
          % (args.agent, args.episodes, wins / args.episodes, wins,
             args.episodes, total_steps / args.episodes,
             1000.0 * total_time / max(total_steps, 1), total_time))


if __name__ == "__main__":
    main()
