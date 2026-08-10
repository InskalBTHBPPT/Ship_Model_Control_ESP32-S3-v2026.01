@echo off
REM OSQP include paths and source list for mpc_common builds
set OSQP_ROOT=%~dp0third_party\osqp
set QDLDL_ROOT=%OSQP_ROOT%\lin_sys\direct\qdldl

set OSQP_INC=-I "%OSQP_ROOT%\include" -I "%QDLDL_ROOT%" -I "%QDLDL_ROOT%\amd\include" -I "%QDLDL_ROOT%\qdldl_sources\include"

set OSQP_SRC=^
  "%OSQP_ROOT%\src\auxil.c" ^
  "%OSQP_ROOT%\src\cs.c" ^
  "%OSQP_ROOT%\src\ctrlc.c" ^
  "%OSQP_ROOT%\src\error.c" ^
  "%OSQP_ROOT%\src\kkt.c" ^
  "%OSQP_ROOT%\src\lin_alg.c" ^
  "%OSQP_ROOT%\src\lin_sys.c" ^
  "%OSQP_ROOT%\src\osqp.c" ^
  "%OSQP_ROOT%\src\polish.c" ^
  "%OSQP_ROOT%\src\proj.c" ^
  "%OSQP_ROOT%\src\scaling.c" ^
  "%OSQP_ROOT%\src\util.c" ^
  "%OSQP_ROOT%\lin_sys\lib_handler.c" ^
  "%QDLDL_ROOT%\qdldl_interface.c" ^
  "%QDLDL_ROOT%\qdldl_sources\src\qdldl.c" ^
  "%QDLDL_ROOT%\amd\src\amd_1.c" ^
  "%QDLDL_ROOT%\amd\src\amd_2.c" ^
  "%QDLDL_ROOT%\amd\src\amd_aat.c" ^
  "%QDLDL_ROOT%\amd\src\amd_control.c" ^
  "%QDLDL_ROOT%\amd\src\amd_defaults.c" ^
  "%QDLDL_ROOT%\amd\src\amd_info.c" ^
  "%QDLDL_ROOT%\amd\src\amd_order.c" ^
  "%QDLDL_ROOT%\amd\src\amd_post_tree.c" ^
  "%QDLDL_ROOT%\amd\src\amd_postorder.c" ^
  "%QDLDL_ROOT%\amd\src\amd_preprocess.c" ^
  "%QDLDL_ROOT%\amd\src\amd_valid.c" ^
  "%QDLDL_ROOT%\amd\src\SuiteSparse_config.c"
