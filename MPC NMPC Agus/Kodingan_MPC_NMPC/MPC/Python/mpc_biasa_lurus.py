"""
Simulasi Linear MPC konvensional — port Python dari MPC/MATLAB/MPC_Biasa_Lurus.m

Jalankan:
    python mpc_biasa_lurus.py
"""

from __future__ import annotations

import time
import warnings

import matplotlib.pyplot as plt
import numpy as np

from mpc_model import (
  MPCConfig,
  ShipParams,
  build_linear_ship_model,
  dimensional_to_nondimensional,
  nondimensional_to_dimensional,
  rk4_step_linear,
)
from mpc_qp import (
  build_mpc_qp_problem,
  build_reference_vector,
  equality_rhs,
  gradient,
  inequality_rhs,
  solve_mpc_step,
)


def run_simulation(
  plot: bool = True,
  show_plots: bool = True,
  log_step_time: bool = True,
) -> dict:
  """Jalankan simulasi LMPC penuh (default 180 detik, Np=60)."""
  ship = ShipParams()
  cfg = MPCConfig()
  l_ship, u_0 = ship.L, ship.u_0

  model = build_linear_ship_model(ship, cfg)
  qp = build_mpc_qp_problem(model, cfg.np_steps, cfg.dt_pred)

  x0_dim = np.array([0.0, 0.0, 0.0, 500.0, 0.0])
  x0_nd = dimensional_to_nondimensional(x0_dim, l_ship, u_0)
  h_ref = np.array([0.0, 0.0, 0.0])

  history_state_nd: list[np.ndarray] = []
  history_input: list[float] = []
  step_times_s: list[float] = []
  qp_iters: list[int] = []
  qp_status: list[str] = []

  u_prev = 0.0
  x_nd = x0_nd.copy()

  num_steps = int(cfg.t_sim_total / cfg.t_sim) + 1
  dt_nd_sim = cfg.t_sim * u_0 / l_ship

  print("Memulai simulasi Linear MPC (Konvensional/Euler)...")
  if log_step_time:
    print(
      f"{'t (s)':>8}  {'waktu (ms)':>10}  {'iter':>6}  {'status':>6}"
    )
    print("-" * 40)

  t_start = time.perf_counter()

  for step in range(num_steps):
    t = step * cfg.t_sim
    step_start = time.perf_counter()

    s_ref_flat = build_reference_vector(qp, t, h_ref, u_0, l_ship)
    f = gradient(qp, s_ref_flat)
    b_eq = equality_rhs(qp, x_nd)
    b_ineq = inequality_rhs(qp, u_prev)

    z_opt, nit, status = solve_mpc_step(qp, f, b_eq, b_ineq)
    if status != "ok":
      warnings.warn(
        f"QP gagal pada t={t} s (iter={nit}): {status}",
        RuntimeWarning,
        stacklevel=2,
      )

    u_apply = float(z_opt[qp.idx_u0])
    u_prev = u_apply

    history_state_nd.append(x_nd.copy())
    history_input.append(u_apply)

    x_nd = rk4_step_linear(
      x_nd,
      u_apply,
      dt_nd_sim,
      model.a_lin,
      model.b_lin,
      model.d_affine,
    )

    step_elapsed = time.perf_counter() - step_start
    step_times_s.append(step_elapsed)
    qp_iters.append(nit)
    qp_status.append(status)

    if log_step_time and (step % 20 == 0 or step == num_steps - 1):
      print(
        f"{t:8.1f}  {step_elapsed * 1000.0:10.2f}  {nit:6d}  {status:>6}"
      )

  elapsed = time.perf_counter() - t_start
  step_times_arr = np.array(step_times_s)

  print("SIMULASI SELESAI")
  print(f"Total Waktu Komputasi MPC Konvensional: {elapsed:.4f} detik")
  if len(step_times_arr) > 0:
    print(
      "Waktu per langkah t - "
      f"min: {step_times_arr.min() * 1000.0:.2f} ms, "
      f"max: {step_times_arr.max() * 1000.0:.2f} ms, "
      f"rata-rata: {step_times_arr.mean() * 1000.0:.2f} ms"
    )

  hist_nd = np.column_stack(history_state_nd)
  hist_dim = np.column_stack([
    nondimensional_to_dimensional(hist_nd[:, i], l_ship, u_0)
    for i in range(hist_nd.shape[1])
  ])
  history_input_arr = np.array(history_input)

  time_vector = np.arange(num_steps, dtype=float) * cfg.t_sim

  x_ref_full = h_ref[0] + time_vector * u_0
  y_ref_full = np.full_like(time_vector, h_ref[1])
  psi_ref_full = np.full_like(time_vector, h_ref[2])

  rmse_x = float(np.sqrt(np.mean((hist_dim[2, :] - x_ref_full) ** 2)))
  rmse_y = float(np.sqrt(np.mean((hist_dim[3, :] - y_ref_full) ** 2)))
  rmse_psi = float(np.sqrt(np.mean((hist_dim[4, :] - psi_ref_full) ** 2)))

  print("\n Hasil Perhitungan RMSE ")
  print(f"RMSE X   : {rmse_x:.4f} meter")
  print(f"RMSE Y   : {rmse_y:.4f} meter")
  print(f"RMSE Psi : {rmse_psi:.4f} rad ({np.rad2deg(rmse_psi):.4f} derajat)\n")

  if plot:
    _plot_results(
      hist_dim,
      history_input_arr,
      time_vector,
      h_ref,
      cfg,
      model,
      show=show_plots,
    )

  return {
    "hist_dim": hist_dim,
    "history_input": history_input_arr,
    "time_vector": time_vector,
    "rmse_x": rmse_x,
    "rmse_y": rmse_y,
    "rmse_psi": rmse_psi,
    "elapsed_s": elapsed,
    "step_times_s": step_times_arr,
    "qp_iters": qp_iters,
    "qp_status": qp_status,
  }


def _plot_results(
  hist_dim: np.ndarray,
  history_input: np.ndarray,
  time_vector: np.ndarray,
  h_ref: np.ndarray,
  cfg: MPCConfig,
  model,
  show: bool = True,
) -> None:
  """Plot trajektori, heading, dan verifikasi batasan (sama seperti MATLAB)."""
  u_limit_deg = cfg.u_limit_deg
  r_limit = cfg.r_limit
  t_sim = cfg.t_sim

  fig1, axes = plt.subplots(2, 1, figsize=(10, 8))
  fig1.canvas.manager.set_window_title("Hasil Simulasi LMPC Konvensional")

  ax1 = axes[0]
  ax1.plot(hist_dim[2, :], hist_dim[3, :], "b-", linewidth=2, label="Kapal")
  x_min, x_max = hist_dim[2, :].min(), hist_dim[2, :].max()
  ax1.plot([x_min, x_max], [0, 0], "r--", linewidth=1.5, label="Referensi")
  ax1.set_xlabel("Posisi X")
  ax1.set_ylabel("Posisi Y")
  ax1.set_title("Trajektori Kapal vs Referensi")
  ax1.legend()
  ax1.grid(True)
  ax1.axis("equal")

  ax2 = axes[1]
  ax2.plot(time_vector, np.rad2deg(hist_dim[4, :]), "b-", linewidth=2, label="Actual")
  ax2.plot(
    time_vector,
    np.full_like(time_vector, np.rad2deg(h_ref[2])),
    "r--",
    linewidth=1.5,
    label="Reff",
  )
  ax2.set_xlabel("Waktu (s)")
  ax2.set_ylabel("Heading (derajat)")
  ax2.set_title("Sudut Haluan (Yaw)")
  ax2.legend()
  ax2.grid(True)

  fig2, axes2 = plt.subplots(3, 1, figsize=(10, 10))
  fig2.canvas.manager.set_window_title("Verifikasi Batasan (Constraints)")

  ax_r = axes2[0]
  ax_r.plot(time_vector, np.rad2deg(hist_dim[1, :]), "b-", linewidth=2, label="Actual r")
  ax_r.axhline(np.rad2deg(r_limit), color="r", linestyle="--", linewidth=1.5, label="Max Limit")
  ax_r.axhline(-np.rad2deg(r_limit), color="r", linestyle="--", linewidth=1.5, label="Min Limit")
  ax_r.set_xlabel("Waktu (s)")
  ax_r.set_ylabel("Yaw Rate (deg/s)")
  ax_r.set_title("Batasan Yaw Rate (r)")
  ax_r.legend()
  ax_r.grid(True)
  ax_r.set_ylim(-np.rad2deg(r_limit) * 1.5, np.rad2deg(r_limit) * 1.5)

  ax_u = axes2[1]
  ax_u.plot(time_vector[: len(history_input)], np.rad2deg(history_input), "g-", linewidth=2)
  ax_u.axhline(u_limit_deg, color="r", linestyle="--", linewidth=1.5)
  ax_u.axhline(-u_limit_deg, color="r", linestyle="--", linewidth=1.5)
  ax_u.set_xlabel("Waktu (s)")
  ax_u.set_ylabel("Sudut Rudder (derajat)")
  ax_u.set_title("Kontrol Input")
  ax_u.grid(True)
  ax_u.set_ylim(-40, 40)

  ax_rate = axes2[2]
  u_deg = np.rad2deg(history_input)
  u_rate = np.concatenate([[0.0], np.diff(u_deg)]) / t_sim
  ax_rate.plot(time_vector[: len(u_rate)], u_rate, "m-", linewidth=2)
  ax_rate.axhline(cfg.u_rate_limit_deg, color="r", linestyle="--", linewidth=1.5, label="Max Rate")
  ax_rate.axhline(-cfg.u_rate_limit_deg, color="r", linestyle="--", linewidth=1.5, label="Min Rate")
  ax_rate.set_xlabel("Waktu (s)")
  ax_rate.set_ylabel("Rudder Rate (deg/s)")
  ax_rate.set_title("Verifikasi Batasan Perubahan Sudut Rudder (derajat)")
  ax_rate.legend()
  ax_rate.grid(True)
  ax_rate.set_ylim(-15, 15)

  plt.tight_layout()
  if show:
    plt.show()


if __name__ == "__main__":
  run_simulation()
