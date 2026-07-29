"""Model kapal dan utilitas nondimensional untuk NMPC."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np


@dataclass
class ShipParams:
    L: float = 101.07
    B: float = 14.0
    T: float = 3.7
    m: float = 2423e3
    u_0: float = 15.4
    C_B: float = 0.65
    x_G: float = 5.25
    A_delta: float = 5.7224
    rho: float = 1024.0
    I_z_nd: float = 1.2392e-4
    u_0_nd: float = 1.0

    A_sys: np.ndarray | None = None
    B_sys: np.ndarray | None = None

    def __post_init__(self) -> None:
        if self.A_sys is None or self.B_sys is None:
            self.A_sys, self.B_sys = build_ship_model(self)


def build_ship_model(params: ShipParams) -> tuple[np.ndarray, np.ndarray]:
    """Hitung matriks dinamika A_sys dan B_sys (sama seperti NMPC_Biasa.m)."""
    L, B, T = params.L, params.B, params.T
    m, C_B, x_G = params.m, params.C_B, params.x_G
    rho = params.rho
    u_0_nd = params.u_0_nd

    Y_v_dot = -(1 + 0.16 * C_B * B / T - 5.1 * (B / L) ** 2) * np.pi * (T / L) ** 2
    Y_r_dot = -(0.67 * (B / L) - 0.0033 * (B / T) ** 2) * np.pi * (T / L) ** 2
    N_v_dot = -(1.1 * B / L - 0.041 * B / T) * np.pi * (T / L) ** 2
    N_r_dot = -((1 / 12) + 0.017 * C_B * B / T - 0.33 * B / L) * np.pi * (T / L) ** 2
    Y_v = -(1 + 0.4 * C_B * B / T) * np.pi * (T / L) ** 2
    Y_r = -(-0.5 + 2.2 * B / L - 0.08 * B / T) * np.pi * (T / L) ** 2
    N_v = -(0.5 + 2.4 * T / L) * np.pi * (T / L) ** 2
    N_r = -(0.25 + 0.039 * B / T - 0.56 * B / L) * np.pi * (T / L) ** 2

    m_nd = 2 * m / (rho * L**3)
    x_G_nd = x_G / L
    I_z_nd = params.I_z_nd

    m_mat = np.array(
        [
            [m_nd - Y_v_dot, m_nd * x_G_nd - Y_r_dot],
            [m_nd * x_G_nd - N_v_dot, I_z_nd - N_r_dot],
        ]
    )
    det_m = np.linalg.det(m_mat)

    a11 = ((I_z_nd - N_r_dot) * Y_v - (m_nd * x_G_nd - Y_r_dot) * N_v) / det_m
    a12 = (
        (I_z_nd - N_r_dot) * (Y_r - m_nd * u_0_nd)
        - (m_nd * x_G_nd - Y_r_dot) * (N_r - m_nd * x_G_nd * u_0_nd)
    ) / det_m
    a21 = ((m_nd - Y_v_dot) * N_v - (m_nd * x_G_nd - N_v_dot) * Y_v) / det_m
    a22 = (
        (m_nd - Y_v_dot) * (N_r - m_nd * x_G_nd * u_0_nd)
        - (m_nd * x_G_nd - N_v_dot) * (Y_r - m_nd * u_0_nd)
    ) / det_m

    a_sys = np.array([[a11, a12], [a21, a22]])
    b_sys = np.array([0.01, 1.0])
    return a_sys, b_sys


def dimensional_to_nondimensional(x_dim: np.ndarray, L: float, u0: float) -> np.ndarray:
    x = np.asarray(x_dim, dtype=float).reshape(5)
    return np.array([x[0] / u0, x[1] * L / u0, x[2] / L, x[3] / L, x[4]])


def nondimensional_to_dimensional(x_nd: np.ndarray, L: float, u0: float) -> np.ndarray:
    x = np.asarray(x_nd, dtype=float).reshape(5)
    return np.array([x[0] * u0, x[1] * u0 / L, x[2] * L, x[3] * L, x[4]])


def ship_dynamics(
    s: np.ndarray,
    u: float,
    a_sys: np.ndarray,
    b_sys: np.ndarray,
    u0_nd: float,
) -> np.ndarray:
    """s = [v; r; x; y; psi] nondimensional."""
    v, r, psi = s[0], s[1], s[4]
    v_r_dot = a_sys @ np.array([v, r]) + b_sys * u
    x_dot = u0_nd * np.cos(psi) - v * np.sin(psi)
    y_dot = u0_nd * np.sin(psi) + v * np.cos(psi)
    return np.array([v_r_dot[0], v_r_dot[1], x_dot, y_dot, r])


def euler_step(
    s: np.ndarray,
    u: float,
    dt: float,
    a_sys: np.ndarray,
    b_sys: np.ndarray,
    u0_nd: float,
) -> np.ndarray:
    s_dot = ship_dynamics(s, u, a_sys, b_sys, u0_nd)
    return s + dt * s_dot
