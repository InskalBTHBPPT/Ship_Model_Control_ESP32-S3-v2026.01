@echo off
setlocal
cd /d "%~dp0"

set OUT=mpc_150.exe
set COMMON=..\mpc_common
set SRC=src\main.cpp
set OBJDIR=build_obj

call "%COMMON%\osqp_flags.bat"

if not exist "%OBJDIR%" mkdir "%OBJDIR%"

echo Compiling OSQP (gcc) ...
gcc -std=c11 -O2 -c %OSQP_INC% ^
  "%COMMON%\third_party\osqp\src\auxil.c" ^
  "%COMMON%\third_party\osqp\src\cs.c" ^
  "%COMMON%\third_party\osqp\src\ctrlc.c" ^
  "%COMMON%\third_party\osqp\src\error.c" ^
  "%COMMON%\third_party\osqp\src\kkt.c" ^
  "%COMMON%\third_party\osqp\src\lin_alg.c" ^
  "%COMMON%\third_party\osqp\src\lin_sys.c" ^
  "%COMMON%\third_party\osqp\src\osqp.c" ^
  "%COMMON%\third_party\osqp\src\polish.c" ^
  "%COMMON%\third_party\osqp\src\proj.c" ^
  "%COMMON%\third_party\osqp\src\scaling.c" ^
  "%COMMON%\third_party\osqp\src\util.c" ^
  "%COMMON%\third_party\osqp\lin_sys\lib_handler.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\qdldl_interface.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\qdldl_sources\src\qdldl.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_1.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_2.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_aat.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_control.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_defaults.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_info.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_order.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_post_tree.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_postorder.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_preprocess.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\amd_valid.c" ^
  "%COMMON%\third_party\osqp\lin_sys\direct\qdldl\amd\src\SuiteSparse_config.c"
if errorlevel 1 (
  echo OSQP compile FAILED
  exit /b 1
)

move /Y *.o "%OBJDIR%\" >nul 2>&1

echo Building %OUT% ...
g++ -std=c++17 -O2 -I "%COMMON%\include" %OSQP_INC% "%SRC%" ^
  "%COMMON%\src\mpc_model.cpp" ^
  "%COMMON%\src\mpc_qp.cpp" ^
  "%COMMON%\src\qp_osqp.cpp" ^
  "%COMMON%\src\qp_solver.cpp" ^
  "%OBJDIR%\*.o" ^
  -o "%OUT%"
if errorlevel 1 (
  echo BUILD FAILED
  exit /b 1
)

echo BUILD OK: %OUT%
endlocal
