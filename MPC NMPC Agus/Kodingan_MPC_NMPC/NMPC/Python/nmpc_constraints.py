"""Fungsi biaya dan kendala NMPC."""

from __future__ import annotations

import numpy as np

from nmpc_model import euler_step


def du_constraints(n: int, u_prev: float, du_max: float) -> tuple[np.ndarray, np.ndarray]:
    """Kendala linear perubahan input: A @ U <= b (sama seperti NMPC_Biasa.m)."""
    a = np.zeros((2 * n, n))
    b = np.zeros(2 * n)

    a[0, 0] = 1.0
    b[0] = u_prev + du_max
    for i in range(1, n):
        a[i, i - 1] = -1.0
        a[i, i] = 1.0
        b[i] = du_max

    a[n, 0] = -1.0
    b[n] = du_max - u_prev
    for i in range(1, n):
        row = n + i
        a[row, i - 1] = 1.0
        a[row, i] = -1.0
        b[row] = du_max

    return a, b


def mpc_cost(
    u: np.ndarray,
    s0: np.ndarray,
    x_ref_seq: np.ndarray,
    y_ref_seq: np.ndarray,
    psi_ref_seq: np.ndarray,
    t_sim: float,
    L: float,
    u0: float,
    a_sys: np.ndarray,
    b_sys: np.ndarray,
    u0_nd: float,
    q: np.ndarray,
    r_weight: float,
) -> float:
    """Fungsi biaya NMPC sequential."""
    s = s0.copy()
    dt_nd = t_sim * u0 / L
    cost = 0.0

    for i in range(len(u)):
        ui = u[i]
        s = euler_step(s, ui, dt_nd, a_sys, b_sys, u0_nd)
        err = np.array([s[2] - x_ref_seq[i], s[3] - y_ref_seq[i], s[4] - psi_ref_seq[i]])
        cost += float(err @ q @ err + r_weight * ui**2)

    return cost


def state_constraints(
    u: np.ndarray,
    s0: np.ndarray,
    t_sim: float,
    L: float,
    u0: float,
    a_sys: np.ndarray,
    b_sys: np.ndarray,
    u0_nd: float,
    r_limit_nd: float,
) -> np.ndarray:
    """Kendala nonlinear yaw rate: c <= 0."""
    s = s0.copy()
    dt_nd = t_sim * u0 / L
    n = len(u)
    c = np.zeros(2 * n)

    idx = 0
    for i in range(n):
        s = euler_step(s, u[i], dt_nd, a_sys, b_sys, u0_nd)
        r = s[1]
        c[idx] = r - r_limit_nd
        c[idx + 1] = -r_limit_nd - r
        idx += 2

    return c
