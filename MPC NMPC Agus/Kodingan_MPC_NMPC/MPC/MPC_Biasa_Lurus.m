clc; clear; close all;

%% Parameter Kapal
L = 101.07;         % Panjang Kapal (m)
B = 14;             % Lebar Kapal (m)
T = 3.7;            % Draft Kapal (m)
m = 2423*1e3;       % Massa Kapal (kg)
u_0 = 15.4;         % Kecepatan surge Kapal (m/s)
C_B = 0.65;         % Koefisien Blok
x_G = 5.25;         % Pusat massa sumbu-x
A_delta = 5.7224;   % Luas rudder (m^2)
rho = 1024;         % Massa jenis air laut (kg/m^3)
r = 0.156*L;        % Kisaran jari-jari girisa

%% Koefisien Hidrodinamika
Y_v_dot = -(1 + 0.16*C_B*B/T - 5.1*(B/L)^2)*pi*(T/L)^2;
Y_r_dot = -(0.67*(B/L) - 0.0033*(B/T)^2)*pi*(T/L)^2;
N_v_dot = -(1.1*B/L - 0.041*B/T)*pi*(T/L)^2;
N_r_dot = -((1/12) + 0.017*C_B*B/T - 0.33*B/L)*pi*(T/L)^2;
Y_v = -(1 + 0.4*C_B*B/T)*pi*(T/L)^2;
Y_r = -(-0.5 + 2.2*B/L - 0.08*B/T)*pi*(T/L)^2;
N_v = -(0.5 + 2.4*T/L)*pi*(T/L)^2;
N_r = -(0.25 + 0.039*B/T - 0.56*B/L)*pi*(T/L)^2;
Y_delta = (rho*pi*A_delta)/(4*L*T);
N_delta = -0.5*Y_delta;

m_nd = 2*m/(rho*L^3);
x_G_nd = x_G/L;
I_z_nd = 1.2392*10^(-4);
u_0_nd = 1;

%% Model Gerak Kapal (Linier)
M = [m_nd-Y_v_dot , m_nd*x_G_nd-Y_r_dot ;
    m_nd*x_G_nd - N_v_dot , I_z_nd - N_r_dot ];

a11 = ((I_z_nd - N_r_dot)*Y_v - (m_nd*x_G_nd - Y_r_dot)*N_v)/det(M);
a12 = ((I_z_nd - N_r_dot)*(Y_r - m_nd*u_0_nd) - (m_nd*x_G_nd - Y_r_dot)*(N_r - m_nd*x_G_nd*u_0_nd))/det(M);
a21 = ((m_nd - Y_v_dot)*N_v - (m_nd*x_G_nd - N_v_dot)*Y_v)/det(M);
a22 = ((m_nd - Y_v_dot)*(N_r - m_nd*x_G_nd*u_0_nd) - (m_nd*x_G_nd - N_v_dot)*(Y_r - m_nd*u_0_nd))/det(M);

A_sys = [a11 , a12 ; a21 , a22 ];
B_sys = [0.01 ; 1];

% State order: [v; r; x; y; psi]
A_lin = zeros(5,5);
A_lin(1:2, 1:2) = A_sys;
A_lin(4, 1) = 1;        
A_lin(4, 5) = u_0_nd;   
A_lin(5, 2) = 1;        

B_lin = zeros(5,1);
B_lin(1:2) = B_sys;

D_affine = zeros(5,1);
D_affine(3) = u_0_nd;   

%% Setup LMPC Konvensional (Beda Hingga)
Tp = 60;            % Horizon waktu prediksi (s)
Np = 60;            % Jumlah langkah prediksi (menggunakan dt = 1 detik)
dt_pred = Tp / Np;  % Interval waktu prediksi diskrit
T_sim = 1;          % Langkah waktu simulasi (s)
T_sim_total = 180;  % Waktu simulasi total (s)

% Diskritisasi Beda Hingga (Euler Forward) ke Non-Dimensional
dt_nd = dt_pred * (u_0 / L);
Ad = eye(5) + A_lin * dt_nd;
Bd = B_lin * dt_nd;
Dd = D_affine * dt_nd;

% Bobot
Q = diag([50, 50, 50]);        
Q_full = diag([0, 0, Q(1,1), Q(2,2), Q(3,3)]); 
R = 100;                      

% Batasan state
r_limit = 0.0932;
r_limit_nd = r_limit * (L / u_0);
lb_state_single = [-Inf; -r_limit_nd; -Inf; -Inf; -Inf];
ub_state_single = [ Inf;  r_limit_nd;  Inf;  Inf;  Inf];

% Batasan input
u_limit = deg2rad(35);
lb_input_single = -u_limit;
ub_input_single =  u_limit;

% Batasan perubahan input (dikalikan dt_pred karena rate adalah per detik)
u_rate_limit = deg2rad(5) * dt_pred;

lb = [repmat(lb_state_single, Np+1, 1); repmat(lb_input_single, Np, 1)];
ub = [repmat(ub_state_single, Np+1, 1); repmat(ub_input_single, Np, 1)];

%% Membangun Matriks QP LMPC Konvensional
% 1. Matriks Hessian (H) 
H_s = kron(eye(Np+1), Q_full);
H_u = kron(eye(Np), R);
H = blkdiag(H_s, H_u);
H = (H + H') / 2; 
H = H + 1e-6 * eye(size(H)); 

% 2. Kendala Persamaan Dinamika Kapal: s(k+1) = Ad*s(k) + Bd*u(k) + Dd
A_eq_dyn_s = kron(eye(Np+1), eye(5));
for i = 1:Np
    A_eq_dyn_s(i*5+1:(i+1)*5, (i-1)*5+1:i*5) = -Ad;
end
A_eq_dyn_u = zeros(5*(Np+1), Np);
for i = 1:Np
    A_eq_dyn_u(i*5+1:(i+1)*5, i) = -Bd;
end
A_eq = [A_eq_dyn_s, A_eq_dyn_u];

b_eq_base = zeros(5*(Np+1), 1);
for i = 1:Np
    b_eq_base(i*5+1:(i+1)*5) = Dd;
end
% Note: Elemen 1-5 dari b_eq_base akan diisi dengan x0 saat di dalam loop.

% 3. Kendala Pertidaksamaan Laju Input (delta u)
A_ineq_rate = zeros(Np, Np);
A_ineq_rate(1,1) = 1;
for i = 2:Np
    A_ineq_rate(i, i) = 1;
    A_ineq_rate(i, i-1) = -1;
end
A_ineq_u = [A_ineq_rate; -A_ineq_rate];
A_ineq = [zeros(2*Np, 5*(Np+1)), A_ineq_u];

%% Simulasi LMPC Konvensional
x0 = [0; 0; 0; 500; 0];         
x0_nd = dimensional_to_nondimensional(x0, L, u_0); 
h_ref = [0; 0; 0];              
history_state_nd = [];
history_input = [];
input_initial = 0;                               

options = optimoptions('quadprog', 'Display', 'none','MaxIterations', 2000);
fprintf('Memulai simulasi Linear MPC (Konvensional/Euler)...\n');
total_timer = tic;

for t = 0:T_sim:T_sim_total
    
    % --- Update Vektor Referensi ---
    ref_future = zeros(5, Np+1); 
    for k = 1:Np+1
        t_predict_sekat = t + (k-1) * dt_pred; 
        ref_future(3, k) = (h_ref(1) + t_predict_sekat * u_0) / L; 
        ref_future(4, k) = h_ref(2) / L;                           
        ref_future(5, k) = h_ref(3);                               
    end
    s_ref_flat = ref_future(:);
    
    % --- Update Vektor Gradien (f) ---
    f_s = -H_s * s_ref_flat;
    f_u = zeros(Np, 1);
    f = [f_s; f_u];
    
    % --- Update Batasan Kondisi Awal ---
    b_eq = b_eq_base;
    b_eq(1:5) = x0_nd; % Mengunci s_0 agar sama dengan kondisi aktual
    
    % --- Update Batasan Laju Input Awal ---
    b_ineq_limit1 = u_rate_limit * ones(Np, 1);
    b_ineq_limit1(1) = u_rate_limit + input_initial; % Batas atas u_0
    
    b_ineq_limit2 = u_rate_limit * ones(Np, 1);
    b_ineq_limit2(1) = u_rate_limit - input_initial; % Batas bawah u_0
    
    b_ineq = [b_ineq_limit1; b_ineq_limit2];
    
    % --- Optimasi QP ---
    [z_opt, ~, exitflag] = quadprog(H, f, A_ineq, b_ineq, A_eq, b_eq, lb, ub, [], options);
    
    if exitflag ~= 1
        warning('Quadprog gagal konvergen pada t = %d. Exitflag: %d', t, exitflag);
    end
    
    % Ambil input pertama (u_0) dari solusi optimal. Terletak setelah vektor state s.
    u_apply = z_opt(5*(Np+1) + 1); 
    input_initial = u_apply;          
    
    % Simpan hasil
    history_state_nd = [history_state_nd, x0_nd];
    history_input = [history_input, u_apply];
    
    % Simulasi dinamika linier kapal (Runge-Kutta 4)
    dt_nd_sim = T_sim * u_0 / L; 
    k1 = linear_ship_dynamics(x0_nd, u_apply, A_lin, B_lin, D_affine);
    k2 = linear_ship_dynamics(x0_nd + 0.5*dt_nd_sim*k1, u_apply, A_lin, B_lin, D_affine);
    k3 = linear_ship_dynamics(x0_nd + 0.5*dt_nd_sim*k2, u_apply, A_lin, B_lin, D_affine);
    k4 = linear_ship_dynamics(x0_nd + dt_nd_sim*k3, u_apply, A_lin, B_lin, D_affine);
    x0_nd = x0_nd + (dt_nd_sim/6) * (k1 + 2*k2 + 2*k3 + k4);
    
    if mod(t, 20) == 0
        disp(['Waktu Simulasi: ', num2str(t), ' s']);
    end
end
waktu_total = toc(total_timer);
fprintf('SIMULASI SELESAI\n');
fprintf('Total Waktu Komputasi MPC Konvensional: %.4f detik\n', waktu_total);

history_state_dim = zeros(size(history_state_nd));
for i = 1:size(history_state_nd, 2)
    history_state_dim(:, i) = nondimensional_to_dimensional(history_state_nd(:, i), L, u_0);
end

%% Plot Hasil (Lengkap dengan Verifikasi Batasan)
figure('Name', 'Hasil Simulasi LMPC Konvensional');
time_vector = 0:T_sim:T_sim_total; 

% Plot 1: Trajektori
subplot(2,1,1);
plot(history_state_dim(3,:), history_state_dim(4,:), 'b-', 'LineWidth', 2); hold on;
plot([min(history_state_dim(3,:)) max(history_state_dim(3,:))], [0 0], 'r--', 'LineWidth', 1.5);
xlabel('Posisi X'); ylabel('Posisi Y');
title('Trajektori Kapal vs Referensi');
legend('Kapal', 'Referensi'); grid on; axis equal; 

% Plot 2: Heading
subplot(2,1,2);
plot(time_vector, rad2deg(history_state_dim(5,:)), 'b-', 'LineWidth', 2); hold on;
y_ref_heading = rad2deg(h_ref(3)) * ones(size(time_vector));
plot(time_vector, y_ref_heading, 'r--', 'LineWidth', 1.5);
xlabel('Waktu (s)'); ylabel('Heading (derajat)');
title('Sudut Haluan (Yaw)');
legend('Actual', 'Reff'); grid on;

%% Plot Verifikasi Batasan (Constraints)
figure('Name', 'Verifikasi Batasan (Constraints)');

% Plot 1: Yaw Rate
subplot(3,1,1);
plot(time_vector, rad2deg(history_state_dim(2, :)), 'b-', 'LineWidth', 2); hold on;
yline(rad2deg(r_limit), 'r--', 'LineWidth', 1.5, 'Label', 'Max Limit');
yline(-rad2deg(r_limit), 'r--', 'LineWidth', 1.5, 'Label', 'Min Limit');
xlabel('Waktu (s)'); ylabel('Yaw Rate (deg/s)');
title('Batasan Yaw Rate (r)'); legend('Actual r', 'Limit'); grid on;
ylim([-rad2deg(r_limit)*1.5, rad2deg(r_limit)*1.5]);

% Plot 2: Rudder Input
subplot(3,1,2);
plot(time_vector, rad2deg(history_input), 'g-', 'LineWidth', 2); hold on;
yline(rad2deg(u_limit), 'r--', 'LineWidth', 1.5);
yline(-rad2deg(u_limit), 'r--', 'LineWidth', 1.5);
xlabel('Waktu (s)'); ylabel('Sudut Rudder (derajat)');
title('Kontrol Input'); legend('Rudder Angle', 'Limit'); grid on; 
ylim([-40 40]);

% Plot 3: Rudder Rate
subplot(3,1,3);
u_actual_deg = rad2deg(history_input); 
% Karena T_sim = 1 detik, perubahan input langsung mencerminkan deg/s
u_rate_calc = [0, diff(u_actual_deg)] / T_sim; 
plot(time_vector, u_rate_calc, 'm-', 'LineWidth', 2); hold on;

% Batas laju putar kemudi fisik adalah 5 derajat/detik
yline(5, 'r--', 'LineWidth', 1.5, 'Label', 'Max Rate');
yline(-5, 'r--', 'LineWidth', 1.5, 'Label', 'Min Rate');
xlabel('Waktu (s)'); ylabel('Rudder Rate (deg/s)');
title('Verifikasi Batasan Perubahan Sudut Rudder (derajat)'); 
legend('Perubahan Input Rudder', 'Limit'); grid on;
ylim([-15 15]);

%% Fungsi Dinamika Linier Kapal
function s_dot = linear_ship_dynamics(s, u, A_lin, B_lin, D_affine)
     s_dot = A_lin * s + B_lin * u + D_affine;
end

%% Fungsi Konversi Nondimensional
function x_nd = dimensional_to_nondimensional(x_dim, L, u_0)
     x_nd = [x_dim(1)/u_0; x_dim(2)*L/u_0; x_dim(3)/L; x_dim(4)/L; x_dim(5)];
end
function x_dim = nondimensional_to_dimensional(x_nd, L, u_0)
     x_dim = [x_nd(1)*u_0; x_nd(2)*u_0/L; x_nd(3)*L; x_nd(4)*L; x_nd(5)];
end