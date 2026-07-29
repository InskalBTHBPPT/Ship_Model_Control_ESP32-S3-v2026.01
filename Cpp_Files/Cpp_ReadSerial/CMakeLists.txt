cmake_minimum_required(VERSION 3.16)
project(cpp_read_serial LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(read_serial
  src/main.cpp
  src/serial_port.cpp
)

target_include_directories(read_serial PRIVATE include)

if(MSVC)
  target_compile_options(read_serial PRIVATE /W4)
else()
  target_compile_options(read_serial PRIVATE -Wall -Wextra -Wpedantic)
endif()
