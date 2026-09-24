cmake_minimum_required(VERSION 3.16)
project(nmpc_simulasi LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(nmpc_sim
  src/sim_main.cpp
  nmpc/nmpc_kapal_waypoint.c
  nmpc/geo_enu.c
  nmpc/waypoint_manager.c
)

target_include_directories(nmpc_sim PRIVATE include nmpc)

if(MSVC)
  target_compile_options(nmpc_sim PRIVATE /W4)
else()
  target_compile_options(nmpc_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(UNIX)
  target_link_libraries(nmpc_sim PRIVATE m)
endif()
