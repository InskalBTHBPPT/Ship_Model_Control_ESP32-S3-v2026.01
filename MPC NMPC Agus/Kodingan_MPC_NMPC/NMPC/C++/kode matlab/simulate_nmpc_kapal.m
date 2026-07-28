function [hist_dim, history_input, time_vector, rmse_data] = simulate_nmpc_kapal()
%#codegen

%% Parameter Kapal
L = 101.07;         
B = 14;             
T = 3.7;            
m = 2423*1e3;       
u_0 = 15.4;         
C_B = 0.65;         
x_G = 5.25;         
A_delta = 5.7224;   
rho = 1024;         
r = 0.156*L;        

%% Koefisien Hidrodinamika
Y_v_dot = -(1 + 0.16*C_B*B/T - 5.1*(B/L)^2)*pi*(T/L)^2;
Y_r_dot = -(0.67*(B/L) - 0.0033*(B/T)^2)*pi*(T/L)^2;
N_v_dot = -(1.1*B/L - 0.041*B/T)*pi*(T/L)^2;
N_r_dot = -((1/12) + 0.017*C_B*B/T - 0.33*B/L)*pi*(T/L)^2;
Y_v = -(1 + 0.4*C_B*B/T)*pi*(T/L)^2;
Y_r = -(-0.5 + 2.2*B/L - 0.08*B/T)*pi*(T/L)^2;
N_v = -(0.5 + 2.4*T/L)*pi*(T/L)^2;
N_r = -(0.25 + 0.039*B/T - 0.56*B/L)*pi*(T/L)^2;

m_nd = 2*m/(rho*L^3);
x_G_nd = x_G/L;
I_z_nd = 1.2392*10^(-4);
u_0_nd = 1;

%% Model Gerak Kapal
M_mat = [m_nd-Y_v_dot , m_nd*x_G_nd-Y_r_dot ;
         m_nd*x_G_nd - N_v_dot , I_z_nd - N_r_dot ];

a11 = ((I_z_nd - N_r_dot)*Y_v - (m_nd*x_G_nd - Y_r_dot)*N_v)/det(M_mat);
a12 = ((I_z_nd - N_r_dot)*(Y_r - m_nd*u_0_nd) - (m_nd*x_G_nd - Y_r_dot)*(N_r - m_nd*x_G_nd*u_0_nd))/det(M_mat);
a21 = ((m_nd - Y_v_dot)*N_v - (m_nd*x_G_nd - N_v_dot)*Y_v)/det(M_mat);
a22 = ((m_nd - Y_v_dot)*(N_r - m_nd*x_G_nd*u_0_nd) - (m_nd*x_G_nd - N_v_dot)*(Y_r - m_nd*u_0_nd))/det(M_mat);

A_sys = [a11 , a12 ;
         a21 , a22 ];
B_sys = [0.01 ;
         1];

%% Setup NMPC
Tp = 30;            
T_sim = 1;          
T_sim_total = 150;  
N = round(Tp / T_sim);   

Q = diag([10, 1, 1]);       
R = 1;                      

r_limit = 0.0932; 
r_limit_nd = r_limit * (L / u_0);
u_limit = deg2rad(35);
u_rate_limit = deg2rad(5);
du_max = u_rate_limit * T_sim;

u_prev = 0; 
[A_du, b_du] = du_constraints(N, u_prev, du_max); 

lb = -u_limit * ones(N,1);
ub =  u_limit * ones(N,1);

s0_dim = [0; 0; 0; 100; 0];         
s0_nd = dimensional_to_nondimensional(s0_dim, L, u_0); 
h_ref = [0; 0; 0]; 

% ========================================================
% PREALOKASI MEMORI UNTUK C++ (PENTING)
% ========================================================
num_steps = round(T_sim_total / T_sim);
history_state_nd = zeros(5, num_steps + 1);
history_input = zeros(1, num_steps + 1);

history_state_nd(:, 1) = s0_nd; 
s_nd = s0_nd;

options = optimoptions('fmincon', 'Algorithm', 'sqp', 'Display', 'none', ...
    'MaxIterations', 200, 'OptimalityTolerance', 1e-6);

for k = 1:num_steps
    t = (k-1) * T_sim;
    
    t_pred = t + (1:N)*T_sim;
    x_ref_seq = (h_ref(1) + t_pred' * u_0) / L;  % Pastikan vektor kolom (N x 1)
    y_ref_seq = h_ref(2) / L * ones(N,1);        
    psi_ref_seq = h_ref(3) * ones(N,1);          
    
    cost_fun = @(U) mpc_cost(U, s_nd, u_prev, x_ref_seq, y_ref_seq, psi_ref_seq, T_sim, L, u_0, A_sys, B_sys, u_0_nd, Q, R);
    nonlcon = @(U) state_constraints(U, s_nd, T_sim, L, u_0, A_sys, B_sys, u_0_nd, r_limit_nd);
    
    U0 = u_prev * ones(N,1);
    
    [U_opt, ~, exitflag] = fmincon(cost_fun, U0, A_du, b_du, [], [], lb, ub, nonlcon, options);

    if exitflag <= 0
        % Menghapus warning() karena I/O string kurang direkomendasikan saat compile C++ murni
        U_opt = U0;
    end
    
    u_applied = U_opt(1); 
    history_input(k) = u_applied; 
    
    dt_nd = T_sim * u_0 / L; 
    s_nd = euler_step(@(s,u) ship_dynamics(s, u, A_sys, B_sys, u_0_nd), s_nd, u_applied, dt_nd);
    history_state_nd(:, k+1) = s_nd; 
    
    u_prev = u_applied;
    [A_du, b_du] = du_constraints(N, u_prev, du_max);
end

time_vector = 0:T_sim:T_sim_total;
history_input(num_steps + 1) = history_input(num_steps); % Mengisi input terakhir

%% Konversi State ke Dimensional
hist_dim = zeros(size(history_state_nd)); 
for i = 1:size(history_state_nd,2) 
    hist_dim(:,i) = nondimensional_to_dimensional(history_state_nd(:,i), L, u_0); 
end

%% Perhitungan RMSE
x_ref_full = h_ref(1) + time_vector * u_0;
y_ref_full = h_ref(2) * ones(size(time_vector));
psi_ref_full = h_ref(3) * ones(size(time_vector));

rmse_x = sqrt(mean((hist_dim(3, :) - x_ref_full).^2));
rmse_y = sqrt(mean((hist_dim(4, :) - y_ref_full).^2));
rmse_psi = sqrt(mean((hist_dim(5, :) - psi_ref_full).^2));

rmse_data = [rmse_x, rmse_y, rmse_psi];

end % Akhir dari fungsi utama

%% --- FUNGSI LOKAL ---
function x_next = euler_step(f, x, u, dt)
    x_next = x + dt * f(x, u);
end

function s_dot = ship_dynamics(s, u, A_sys, B_sys, u0_nd)
    v = s(1); r = s(2); psi = s(5);
    v_r_dot = A_sys * [v; r] + B_sys * u;
    x_dot = u0_nd*cos(psi) - v*sin(psi);
    y_dot = u0_nd*sin(psi) + v*cos(psi);
    s_dot = [v_r_dot(1); v_r_dot(2); x_dot; y_dot; r];
end

function J = mpc_cost(U, s0, u_prev, x_ref_seq, y_ref_seq, psi_ref_seq, T_sim, L, u0, A_sys, B_sys, u0_nd, Q, R)
    N = length(U); 
    s = s0;
    dt_nd = T_sim * u0 / L;
    J = 0; 
    
    for i = 1:N
        u = U(i);
        s = euler_step(@(s,u) ship_dynamics(s, u, A_sys, B_sys, u0_nd), s, u, dt_nd);
        err = [s(3) - x_ref_seq(i);
               s(4) - y_ref_seq(i);
               s(5) - psi_ref_seq(i)];
        J = J + err' * Q * err + R * u^2;
    end
end

function [c, ceq] = state_constraints(U, s0, T_sim, L, u0, A_sys, B_sys, u0_nd, r_limit_nd)
    N = length(U);
    s = s0;
    dt_nd = T_sim * u0 / L;
    c = zeros(2*N, 1); % Prealokasi kendala
    
    idx = 1;
    for i = 1:N
        s = euler_step(@(s,u) ship_dynamics(s, u, A_sys, B_sys, u0_nd), s, U(i), dt_nd);
        r = s(2);    
        c(idx) = r - r_limit_nd;
        c(idx+1) = -r_limit_nd - r;
        idx = idx + 2;
    end
    ceq = [];
end

function [A, b] = du_constraints(N, u_prev, du_max)
    A = zeros(2*N, N);
    b = zeros(2*N, 1);
    
    A(1,1) = 1;   b(1) = u_prev + du_max;
    for i = 2:N
        A(i, i-1) = -1;   A(i, i) = 1;
        b(i) = du_max;
    end
    
    A(N+1,1) = -1;   b(N+1) = du_max - u_prev;
    for i = 2:N
        A(N+i, i-1) = 1;   A(N+i, i) = -1;
        b(N+i) = du_max;
    end
end

function x_nd = dimensional_to_nondimensional(x, L, u0)
    x_nd = [x(1)/u0; x(2)*L/u0; x(3)/L; x(4)/L; x(5)];
end

function x_dim = nondimensional_to_dimensional(x, L, u0)
    x_dim = [x(1)*u0; x(2)*u0/L; x(3)*L; x(4)*L; x(5)];
end