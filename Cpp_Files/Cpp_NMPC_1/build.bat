@echo off
setlocal
cd /d "%~dp0"

set OUT=nmpc_1.exe
set INC=lib\nmpc
set SRC=src\main.cpp

echo Building %OUT% ...
g++ -std=c++17 -O2 -fopenmp -I "%INC%" "%SRC%" "%INC%\*.cpp" -o "%OUT%"
if errorlevel 1 (
  echo BUILD FAILED
  exit /b 1
)

echo BUILD OK: %OUT%
endlocal
