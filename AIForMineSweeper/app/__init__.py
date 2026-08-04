# -*- coding: utf-8 -*-
"""AIForMineSweeper 求解服务包。"""

__version__ = "1.1.0"

from .config import DEFAULT_CONFIG, get_api_key, load_config
from .solvers import solve_heuristic, solve_payload, solve_with_deepseek
from .validation import validate_move, validate_payload

__all__ = [
    "DEFAULT_CONFIG",
    "load_config",
    "get_api_key",
    "validate_move",
    "validate_payload",
    "solve_payload",
    "solve_with_deepseek",
    "solve_heuristic",
]
