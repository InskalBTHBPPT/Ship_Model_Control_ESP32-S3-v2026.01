"""Pembentukan dan penyelesaian QP LMPC konvensional (formulasi penuh seperti MATLAB)."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
import quadprog

from mpc_model import LinearShipModel


@dataclass
class MPCQPProblem:
  """Matriks QP LMPC — struktur tetap, f dan b_eq di-update tiap langkah."""

  np_steps: int
  n_state: int
  h: np.ndarray
  h_s: np.ndarray
  a_eq: np.ndarray
  a_ineq: np.ndarray
  b_eq_base: np.ndarray
  lb: np.ndarray
  ub: np.ndarray
  n_var: int
  idx_u0: int
  dt_pred: float = 1.0
  u_rate_limit: float = 0.0


def build_mpc_qp_problem(model: LinearShipModel, np_steps: int, dt_pred: float) -> MPCQPProblem:
  """Bangun H, A_eq, A_ineq, batas — sama seperti MPC_Biasa_Lurus.m."""
  n_state = model.n_state
  ad, bd, dd = model.ad, model.bd, model.dd
  q_full = model.q_full
  r_weight = model.r_weight

  h_s = np.kron(np.eye(np_steps + 1), q_full)
  h_u = np.kron(np.eye(np_steps), np.array([[r_weight]]))
  h = np.block([
    [h_s, np.zeros((h_s.shape[0], h_u.shape[1]))],
    [np.zeros((h_u.shape[0], h_s.shape[0])), h_u],
  ])
  h = 0.5 * (h + h.T) + 1e-6 * np.eye(h.shape[0])

  a_eq_dyn_s = np.kron(np.eye(np_steps + 1), np.eye(n_state))
  for i in range(1, np_steps + 1):
    row = slice(i * n_state, (i + 1) * n_state)
    col_prev = slice((i - 1) * n_state, i * n_state)
    a_eq_dyn_s[row, col_prev] = -ad

  a_eq_dyn_u = np.zeros((n_state * (np_steps + 1), np_steps))
  for i in range(np_steps):
    row = slice((i + 1) * n_state, (i + 2) * n_state)
    a_eq_dyn_u[row, i] = -bd

  a_eq = np.hstack([a_eq_dyn_s, a_eq_dyn_u])

  b_eq_base = np.zeros(n_state * (np_steps + 1))
  for i in range(np_steps):
    row = slice((i + 1) * n_state, (i + 2) * n_state)
    b_eq_base[row] = dd

  a_ineq_rate = np.zeros((np_steps, np_steps))
  a_ineq_rate[0, 0] = 1.0
  for i in range(1, np_steps):
    a_ineq_rate[i, i] = 1.0
    a_ineq_rate[i, i - 1] = -1.0
  a_ineq_u = np.vstack([a_ineq_rate, -a_ineq_rate])
  a_ineq = np.hstack([
    np.zeros((2 * np_steps, n_state * (np_steps + 1))),
    a_ineq_u,
  ])

  lb_state = np.array([-np.inf, -model.r_limit_nd, -np.inf, -np.inf, -np.inf])
  ub_state = np.array([np.inf, model.r_limit_nd, np.inf, np.inf, np.inf])
  lb = np.concatenate([
    np.tile(lb_state, np_steps + 1),
    np.full(np_steps, -model.u_limit),
  ])
  ub = np.concatenate([
    np.tile(ub_state, np_steps + 1),
    np.full(np_steps, model.u_limit),
  ])

  n_var = n_state * (np_steps + 1) + np_steps
  return MPCQPProblem(
    np_steps=np_steps,
    n_state=n_state,
    h=h,
    h_s=h_s,
    a_eq=a_eq,
    a_ineq=a_ineq,
    b_eq_base=b_eq_base,
    lb=lb,
    ub=ub,
    n_var=n_var,
    idx_u0=n_state * (np_steps + 1),
    dt_pred=dt_pred,
    u_rate_limit=model.u_rate_limit,
  )


def build_reference_vector(
  problem: MPCQPProblem,
  t: float,
  h_ref: np.ndarray,
  u_0: float,
  l_ship: float,
) -> np.ndarray:
  ref_future = np.zeros((problem.n_state, problem.np_steps + 1))
  for k in range(problem.np_steps + 1):
    t_predict = t + k * problem.dt_pred
    ref_future[2, k] = (h_ref[0] + t_predict * u_0) / l_ship
    ref_future[3, k] = h_ref[1] / l_ship
    ref_future[4, k] = h_ref[2]
  return ref_future.reshape(-1, order="F")


def gradient(problem: MPCQPProblem, s_ref_flat: np.ndarray) -> np.ndarray:
  f_s = -problem.h_s @ s_ref_flat
  f_u = np.zeros(problem.np_steps)
  return np.concatenate([f_s, f_u])


def equality_rhs(problem: MPCQPProblem, x0_nd: np.ndarray) -> np.ndarray:
  b_eq = problem.b_eq_base.copy()
  b_eq[: problem.n_state] = x0_nd
  return b_eq


def inequality_rhs(problem: MPCQPProblem, u_prev: float) -> np.ndarray:
  limit = problem.u_rate_limit
  b1 = np.full(problem.np_steps, limit)
  b1[0] = limit + u_prev
  b2 = np.full(problem.np_steps, limit)
  b2[0] = limit - u_prev
  return np.concatenate([b1, b2])


def solve_mpc_step(
  problem: MPCQPProblem,
  f: np.ndarray,
  b_eq: np.ndarray,
  b_ineq: np.ndarray,
) -> tuple[np.ndarray, int, str]:
  """
  Selesaikan QP padanan quadprog MATLAB via quadprog (Python).

  min 1/2 z'Hz - a'z  s.t.  C'z >= b,  A_eq z = b_eq,  lb <= z <= ub
  """
  n = problem.n_var
  meq = problem.a_eq.shape[0]

  c = np.hstack([
    problem.a_eq.T,
    (-problem.a_ineq).T,
    np.eye(n),
    -np.eye(n),
  ])
  b_qp = np.concatenate([b_eq, -b_ineq, problem.lb, -problem.ub])

  try:
    result = quadprog.solve_qp(
      problem.h,
      -f,
      c,
      b_qp,
      meq=meq,
    )
    z_opt = np.asarray(result[0]).reshape(-1)
    return z_opt, 0, "ok"
  except (ValueError, TypeError) as exc:
    return np.zeros(n), 0, f"fail: {exc}"
