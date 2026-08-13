@echo off
REM Auto-start helper: sesuaikan COM_PORT sebelum deploy ke mini PC.
set COM_PORT=COM16
set BAUD=115200
set RUDDER_MODE=yawrate2
REM stdout: all | csv | wp | none  (lihat README --print)
set PRINT_MODE=none
REM Tunggu USB serial siap setelah boot (detik)
set USB_WAIT_SEC=15

cd /d "%~dp0"

if not exist "read_write_serial.exe" (
  echo [ERROR] read_write_serial.exe tidak ditemukan di %~dp0
  pause
  exit /b 1
)

timeout /t %USB_WAIT_SEC% /nobreak >nul

:retry
read_write_serial.exe --port %COM_PORT% --baud %BAUD% --rudder-mode %RUDDER_MODE% --print %PRINT_MODE%
echo.
echo [WARN] Program berhenti. Restart dalam 5 detik...
timeout /t 5 /nobreak >nul
goto retry
