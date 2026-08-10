cmake_minimum_required(VERSION 3.16)
project(cpp_nmpc_1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB NMPC_SOURCES CONFIGURE_DEPENDS lib/nmpc/*.cpp)

add_executable(nmpc_1 src/main.cpp ${NMPC_SOURCES})

target_include_directories(nmpc_1 PRIVATE lib/nmpc)

if(MSVC)
  target_compile_options(nmpc_1 PRIVATE /W4 /O2)
else()
  target_compile_options(nmpc_1 PRIVATE -Wall -Wextra -O2 -fopenmp)
  target_link_options(nmpc_1 PRIVATE -fopenmp)
endif()
