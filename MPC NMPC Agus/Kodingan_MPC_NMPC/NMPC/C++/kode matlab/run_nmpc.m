clc; clear; close all;

fprintf('Memulai simulasi NMPC...\n');
timer_val = tic;

% Memanggil fungsi yang sudah diubah (atau file MEX-nya jika sudah di-generate)
[hist_dim, history_input, time_vector, rmse_data] = simulate_nmpc_kapal();

waktu = toc(timer_val);
fprintf('SIMULASI SELESAI\n');
fprintf('Total Waktu Komputasi: %.4f detik\n', waktu);

fprintf('\n Hasil Perhitungan RMSE \n');
fprintf('RMSE X   : %.4f meter\n', rmse_data(1));
fprintf('RMSE Y   : %.4f meter\n', rmse_data(2));
fprintf('RMSE Psi : %.4f rad (%.4f derajat)\n\n', rmse_data(3), rad2deg(rmse_data(3)));

% --- Letakkan semua kode plotting figure 1 sampai 4 kamu di sini persis seperti aslinya ---