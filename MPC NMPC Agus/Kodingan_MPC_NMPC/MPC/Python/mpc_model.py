"""Model linier kapal dan utilitas untuk LMPC konvensional (MPC_Biasa_Lurus.m)."""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np

_NMPC_PYTHON = Path(__file__).resolve().parents[2] / "NMPC" / "Python"
if str(_NMPC_PYTHON) not in sys.path:
  sys.path.insert(0, str(_NMPC_PYTHON))

from nmpc_model import (
    ShipParams,
    build_ship_model,
    dimensional_to_nondimensional,
    nondimensional_to_dimensional,
)

__all__ = [
    "ShipParams",
    "MPCConfig",
    "LinearShipModel",
    "build_linear_ship_model",
    "dimensional_to_nondimensional",
    "nondimensional_to_dimensional",
    "linear_ship_dynamics",
    "rk4_step_linear",
]


@dataclass
class MPCConfig:
  """Parameter simulasi MPC — sama seperti MPC_Biasa_Lurus.m."""

  tp: float = 60.0
  np_steps: int = 60
  t_sim: float = 1.0
  t_sim_total: float = 180.0
  q_diag: tuple[float, float, float] = (50.0, 50.0, 50.0)
  r_weight: float = 100.0
  r_limit: float = 0.0932
  u_limit_deg: float = 35.0
  u_rate_limit_deg: float = 5.0

  @property
  def dt_pred(self) -> float:
    return self.tp / self.np_steps


@dataclass
class LinearShipModel:
  """Model state-space linier [v, r, x, y, psi] nondimensional."""

  a_lin: np.ndarray
  b_lin: np.ndarray
  d_affine: np.ndarray
  ad: np.ndarray
  bd: np.ndarray
  dd: np.ndarray
  q_full: np.ndarray
  r_weight: float
  r_limit_nd: float
  u_limit: float
  u_rate_limit: float

  n_state: int = 5


def build_linear_ship_model(
  ship: ShipParams,
  cfg: MPCConfig,
) -> LinearShipModel:
  """Bangun A_lin, B_lin, D_affine dan diskretisasi Euler forward."""
  a_sys, b_sys = ship.A_sys, ship.B_sys
  u_0_nd = ship.u_0_nd
  l_ship, u_0 = ship.L, ship.u_0

  a_lin = np.zeros((5, 5))
  a_lin[:2, :2] = a_sys
  a_lin[3, 0] = 1.0
  a_lin[3, 4] = u_0_nd
  a_lin[4, 1] = 1.0

  b_lin = np.zeros(5)
  b_lin[:2] = b_sys

  d_affine = np.zeros(5)
  d_affine[2] = u_0_nd

  dt_nd = cfg.dt_pred * (u_0 / l_ship)
  ad = np.eye(5) + a_lin * dt_nd
  bd = b_lin * dt_nd
  dd = d_affine * dt_nd

  q = np.diag(cfg.q_diag)
  q_full = np.diag([0.0, 0.0, q[0, 0], q[1, 1], q[2, 2]])

  r_limit_nd = cfg.r_limit * (l_ship / u_0)
  u_limit = np.deg2rad(cfg.u_limit_deg)
  u_rate_limit = np.deg2rad(cfg.u_rate_limit_deg) * cfg.dt_pred

  return LinearShipModel(
    a_lin=a_lin,
    b_lin=b_lin,
    d_affine=d_affine,
    ad=ad,
    bd=bd,
    dd=dd,
    q_full=q_full,
    r_weight=cfg.r_weight,
    r_limit_nd=r_limit_nd,
    u_limit=u_limit,
    u_rate_limit=u_rate_limit,
  )


def linear_ship_dynamics(
  s: np.ndarray,
  u: float,
  a_lin: np.ndarray,
  b_lin: np.ndarray,
  d_affine: np.ndarray,
) -> np.ndarray:
  """s_dot = A_lin s + B_lin u + D_affine."""
  return a_lin @ s + b_lin * u + d_affine


def rk4_step_linear(
  s: np.ndarray,
  u: float,
  dt_nd: float,
  a_lin: np.ndarray,
  b_lin: np.ndarray,
  d_affine: np.ndarray,
) -> np.ndarray:
  """Integrasi RK4 plant linier (sama seperti loop simulasi MATLAB)."""
  k1 = linear_ship_dynamics(s, u, a_lin, b_lin, d_affine)
  k2 = linear_ship_dynamics(s + 0.5 * dt_nd * k1, u, a_lin, b_lin, d_affine)
  k3 = linear_ship_dynamics(s + 0.5 * dt_nd * k2, u, a_lin, b_lin, d_affine)
  k4 = linear_ship_dynamics(s + dt_nd * k3, u, a_lin, b_lin, d_affine)
  return s + (dt_nd / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4)
