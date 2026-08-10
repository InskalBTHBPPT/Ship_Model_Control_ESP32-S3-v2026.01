@echo off
setlocal
cd /d "%~dp0"

set OUT=mpc_150.exe
set COMMON=..\mpc_common
set SRC=src\main.cpp

echo Building %OUT% ...
g++ -std=c++17 -O2 -I "%COMMON%\include" "%SRC%" ^
  "%COMMON%\src\mpc_model.cpp" ^
  "%COMMON%\src\mpc_qp.cpp" ^
  "%COMMON%\src\qp_solver.cpp" ^
  -o "%OUT%"
if errorlevel 1 (
  echo BUILD FAILED
  exit /b 1
)

echo BUILD OK: %OUT%
endlocal
