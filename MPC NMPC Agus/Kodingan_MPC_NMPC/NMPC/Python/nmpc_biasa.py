"""
Simulasi NMPC — port Python dari NMPC/MATLAB/NMPC_Biasa.m

Jalankan:
    python nmpc_biasa.py
"""

from __future__ import annotations

import time
import warnings

import matplotlib.pyplot as plt
import numpy as np
from scipy.optimize import LinearConstraint, NonlinearConstraint, minimize

from nmpc_constraints import du_constraints, mpc_cost, state_constraints
from nmpc_model import (
    ShipParams,
    dimensional_to_nondimensional,
    euler_step,
    nondimensional_to_dimensional,
)


class _StepOptimizerStats:
    """Counter evaluasi optimizer untuk satu langkah waktu t."""

    __slots__ = ("mpc_cost_calls", "constraint_calls")

    def __init__(self) -> None:
        self.mpc_cost_calls = 0
        self.constraint_calls = 0

    def reset(self) -> None:
        self.mpc_cost_calls = 0
        self.constraint_calls = 0

    @property
    def total_propagations(self) -> int:
        return self.mpc_cost_calls + self.constraint_calls


def run_simulation(
    plot: bool = True,
    show_plots: bool = True,
    log_step_time: bool = True,
) -> dict:
    """Jalankan simulasi NMPC penuh 150 detik."""
    ship = ShipParams()
    L, u_0 = ship.L, ship.u_0
    a_sys, b_sys = ship.A_sys, ship.B_sys
    u_0_nd = ship.u_0_nd

    tp = 30.0
    t_sim = 1.0
    t_sim_total = 150.0
    n = int(round(tp / t_sim))

    q = np.diag([10.0, 1.0, 1.0])
    r_weight = 1.0

    r_limit = 0.0932
    r_limit_nd = r_limit * (L / u_0)
    u_limit = np.deg2rad(35.0)
    u_rate_limit = np.deg2rad(5.0)
    du_max = u_rate_limit * t_sim

    u_prev = 0.0
    a_du, b_du = du_constraints(n, u_prev, du_max)
    lb = np.full(n, -u_limit)
    ub = np.full(n, u_limit)
    bounds = list(zip(lb, ub))

    s0_dim = np.array([0.0, 0.0, 0.0, 100.0, 0.0])
    s0_nd = dimensional_to_nondimensional(s0_dim, L, u_0)
    h_ref = np.array([0.0, 0.0, 0.0])

    history_state_nd = [s0_nd.copy()]
    history_input: list[float] = []
    step_times_s: list[float] = []
    slsqp_iters: list[int] = []
    mpc_cost_calls_log: list[int] = []
    constraint_calls_log: list[int] = []
    total_propagations_log: list[int] = []
    opt_stats = _StepOptimizerStats()
    s_nd = s0_nd.copy()

    num_steps = int(t_sim_total / t_sim)
    print("Memulai simulasi NMPC")
    if log_step_time:
        print(
            f"{'t (s)':>8}  {'waktu (ms)':>10}  {'SLSQP':>6}  {'mpc_cost':>9}  "
            f"{'kendala':>8}  {'prop_tot':>9}  {'status':>6}"
        )
        print(f"{'':>8}  {'':>10}  {'':>6}  {'':>9}  {'':>8}  {f'(N={n})':>9}")
        print("-" * 68)
    t_start = time.perf_counter()

    for step in range(num_steps):
        t = step * t_sim
        step_start = time.perf_counter()
        opt_stats.reset()

        t_pred = t + (np.arange(1, n + 1)) * t_sim
        x_ref_seq = (h_ref[0] + t_pred * u_0) / L
        y_ref_seq = np.full(n, h_ref[1] / L)
        psi_ref_seq = np.full(n, h_ref[2])

        u0_guess = np.full(n, u_prev)

        def objective(u_vec: np.ndarray) -> float:
            opt_stats.mpc_cost_calls += 1
            return mpc_cost(
                u_vec,
                s_nd,
                x_ref_seq,
                y_ref_seq,
                psi_ref_seq,
                t_sim,
                L,
                u_0,
                a_sys,
                b_sys,
                u_0_nd,
                q,
                r_weight,
            )

        def nonlinear_ineq(u_vec: np.ndarray) -> np.ndarray:
            opt_stats.constraint_calls += 1
            return state_constraints(
                u_vec, s_nd, t_sim, L, u_0, a_sys, b_sys, u_0_nd, r_limit_nd
            )

        constraints = [
            LinearConstraint(a_du, -np.inf, b_du),
            NonlinearConstraint(nonlinear_ineq, -np.inf, 0.0),
        ]

        result = minimize(
            objective,
            u0_guess,
            method="SLSQP",
            bounds=bounds,
            constraints=constraints,
            options={"maxiter": 200, "ftol": 1e-6, "disp": False},
        )

        if not result.success:
            warnings.warn(
                f"SLSQP gagal pada t={t:.1f} s: {result.message}",
                RuntimeWarning,
                stacklevel=2,
            )
            u_opt = u0_guess
        else:
            u_opt = result.x

        u_applied = float(u_opt[0])
        history_input.append(u_applied)

        dt_nd = t_sim * u_0 / L
        s_nd = euler_step(s_nd, u_applied, dt_nd, a_sys, b_sys, u_0_nd)
        history_state_nd.append(s_nd.copy())

        u_prev = u_applied
        a_du, b_du = du_constraints(n, u_prev, du_max)

        step_elapsed = time.perf_counter() - step_start
        step_times_s.append(step_elapsed)

        slsqp_iter = int(getattr(result, "nit", 0))
        mpc_cost_count = opt_stats.mpc_cost_calls
        constraint_count = opt_stats.constraint_calls
        propagations_in_opt = opt_stats.total_propagations * n

        slsqp_iters.append(slsqp_iter)
        mpc_cost_calls_log.append(mpc_cost_count)
        constraint_calls_log.append(constraint_count)
        total_propagations_log.append(propagations_in_opt)

        if log_step_time:
            status = "OK" if result.success else "FAIL"
            print(
                f"{t:8.1f}  {step_elapsed * 1000.0:10.2f}  {slsqp_iter:6d}  "
                f"{mpc_cost_count:9d}  {constraint_count:8d}  "
                f"{propagations_in_opt:9d}  {status:>6}"
            )

    elapsed = time.perf_counter() - t_start
    step_times_arr = np.array(step_times_s, dtype=float)
    slsqp_iters_arr = np.array(slsqp_iters, dtype=int)
    mpc_cost_calls_arr = np.array(mpc_cost_calls_log, dtype=int)
    constraint_calls_arr = np.array(constraint_calls_log, dtype=int)
    total_propagations_arr = np.array(total_propagations_log, dtype=int)
    print("SIMULASI SELESAI")
    print(f"Total Waktu Komputasi: {elapsed:.4f} detik")
    if len(step_times_arr) > 0:
        print(
            "Waktu per langkah t - "
            f"min: {step_times_arr.min() * 1000.0:.2f} ms, "
            f"max: {step_times_arr.max() * 1000.0:.2f} ms, "
            f"rata-rata: {step_times_arr.mean() * 1000.0:.2f} ms"
        )
        print(
            "SLSQP per t - "
            f"min: {slsqp_iters_arr.min()}, max: {slsqp_iters_arr.max()}, "
            f"rata-rata: {slsqp_iters_arr.mean():.1f}"
        )
        print(
            "mpc_cost per t - "
            f"min: {mpc_cost_calls_arr.min()}, max: {mpc_cost_calls_arr.max()}, "
            f"rata-rata: {mpc_cost_calls_arr.mean():.1f}"
        )
        print(
            f"Propagasi horizon (N={n} per evaluasi) - "
            f"min: {total_propagations_arr.min()}, max: {total_propagations_arr.max()}, "
            f"total seluruh simulasi: {total_propagations_arr.sum()}"
        )
        print(
            f"(+ {num_steps} propagasi nyata kapal, 1x Euler per langkah t di luar optimizer)"
        )

    history_state_nd_arr = np.column_stack(history_state_nd)
    history_input_arr = np.array(history_input, dtype=float)
    history_input_plot = np.append(history_input_arr, history_input_arr[-1])

    time_vector = np.arange(0.0, t_sim_total + t_sim, t_sim)

    hist_dim = np.column_stack(
        [
            nondimensional_to_dimensional(history_state_nd_arr[:, i], L, u_0)
            for i in range(history_state_nd_arr.shape[1])
        ]
    )

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

    step_time_vector = np.arange(0.0, t_sim_total, t_sim)

    if plot:
        _plot_results(
            hist_dim,
            history_input_plot,
            time_vector,
            r_limit,
            u_limit,
            u_rate_limit,
            t_sim,
            step_time_vector,
            step_times_arr,
            show=show_plots,
        )

    return {
        "rmse_x": rmse_x,
        "rmse_y": rmse_y,
        "rmse_psi": rmse_psi,
        "hist_dim": hist_dim,
        "history_input": history_input_plot,
        "time_vector": time_vector,
        "step_time_vector": step_time_vector,
        "step_times_s": step_times_arr,
        "slsqp_iters": slsqp_iters_arr,
        "mpc_cost_calls": mpc_cost_calls_arr,
        "constraint_calls": constraint_calls_arr,
        "total_propagations": total_propagations_arr,
        "horizon_n": n,
        "elapsed_s": elapsed,
    }


def _plot_results(
    hist_dim: np.ndarray,
    history_input: np.ndarray,
    time_vector: np.ndarray,
    r_limit: float,
    u_limit: float,
    u_rate_limit: float,
    t_sim: float,
    step_time_vector: np.ndarray,
    step_times_s: np.ndarray,
    show: bool = True,
) -> None:
    """Empat grafik setara NMPC_Biasa.m."""
    fig1, ax1 = plt.subplots(figsize=(8, 5))
    ax1.plot(hist_dim[2, :], hist_dim[3, :], "b-", linewidth=2, label="NMPC")
    ax1.plot(hist_dim[2, 0], hist_dim[3, 0], "o", markersize=10, markeredgecolor="k", markerfacecolor="k", label="Titik Awal")
    ax1.plot(hist_dim[2, -1], hist_dim[3, -1], "s", markersize=10, markeredgecolor="y", markerfacecolor="y", label="Titik Akhir NMPC")
    ax1.plot([0, 2800], [0, 0], "r--", linewidth=1.5, label="Lintasan Referensi")
    ax1.set_xlabel("Posisi X (m)")
    ax1.set_ylabel("Posisi Y (m)")
    ax1.set_title("Lintasan Kapal")
    ax1.set_xlim(0, 2800)
    ax1.set_ylim(-100, 600)
    ax1.legend()
    ax1.grid(True)

    fig2, ax2 = plt.subplots(figsize=(8, 4))
    ax2.plot(time_vector, np.rad2deg(hist_dim[1, :]), "b-", linewidth=2, label="NMPC")
    ax2.axhline(np.rad2deg(r_limit), color="r", linestyle="--", linewidth=1.5)
    ax2.axhline(-np.rad2deg(r_limit), color="r", linestyle="--", linewidth=1.5)
    ax2.set_xlabel("Waktu (s)")
    ax2.set_ylabel("Yaw Rate (derajat/s)")
    ax2.set_title("Grafik Yaw Rate")
    ax2.set_ylim(-np.rad2deg(r_limit) * 1.5, np.rad2deg(r_limit) * 1.5)
    ax2.legend(["NMPC", "Batasan Yaw Rate"])
    ax2.grid(True)

    fig3, ax3 = plt.subplots(figsize=(8, 4))
    ax3.plot(time_vector, np.rad2deg(history_input), "b-", linewidth=2, label="NMPC")
    ax3.axhline(np.rad2deg(u_limit), color="r", linestyle="--", linewidth=1.5)
    ax3.axhline(-np.rad2deg(u_limit), color="r", linestyle="--", linewidth=1.5)
    ax3.set_xlabel("Waktu (s)")
    ax3.set_ylabel("Sudut Rudder (derajat)")
    ax3.set_title("Input (u)")
    ax3.set_ylim(-40, 40)
    ax3.legend(["NMPC", "Batasan Input"])
    ax3.grid(True)

    fig4, ax4 = plt.subplots(figsize=(8, 4))
    u_changes = np.concatenate([[0.0], np.diff(np.rad2deg(history_input))])
    u_rate = u_changes / t_sim
    ax4.plot(time_vector, u_rate, "b-", linewidth=2, label="NMPC")
    ax4.axhline(np.rad2deg(u_rate_limit), color="r", linestyle="--", linewidth=1.5)
    ax4.axhline(-np.rad2deg(u_rate_limit), color="r", linestyle="--", linewidth=1.5)
    ax4.set_xlabel("Waktu (s)")
    ax4.set_ylabel("Perubahan Sudut Rudder (derajat)")
    ax4.set_title("Perubahan Input (Δu)")
    ax4.set_ylim(-10, 10)
    ax4.legend(["NMPC", "Batasan Perubahan Input"])
    ax4.grid(True)

    fig5, ax5 = plt.subplots(figsize=(8, 4))
    ax5.plot(step_time_vector, step_times_s * 1000.0, "b-", linewidth=1.5, label="Waktu solve")
    ax5.set_xlabel("Waktu simulasi t (s)")
    ax5.set_ylabel("Waktu komputasi (ms)")
    ax5.set_title("Waktu Aktual per Langkah t")
    ax5.grid(True)
    ax5.legend()

    if show:
        plt.show()


if __name__ == "__main__":
    run_simulation(plot=True, show_plots=True)
