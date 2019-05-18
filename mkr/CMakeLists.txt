cmake_minimum_required(VERSION 3.14)
project(mkr C)
set(CMAKE_C_STANDARD 99)
SET(CMAKE_C_COMPILER /usr/bin/mpicc)
add_executable(mkr string.c mpi_string.h)
target_link_libraries(mkr m)
target_link_libraries(mkr mpi)

