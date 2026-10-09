cmake_minimum_required(VERSION 3.16)
project(cpp_read_write_serial_nmpc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(read_write_serial
  src/main.cpp
  src/serial_port.cpp
  nmpc/nmpc_kapal_waypoint.c
  nmpc/geo_enu.c
  nmpc/waypoint_manager.c
)

target_include_directories(read_write_serial PRIVATE include nmpc)

if(MSVC)
  target_compile_options(read_write_serial PRIVATE /W4)
else()
  target_compile_options(read_write_serial PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(UNIX)
  target_link_libraries(read_write_serial PRIVATE m)
endif()
