cmake_minimum_required(VERSION 3.14)
project(mkr C)

#find_package(MPI REQUIRED)
set(CMAKE_C_STANDARD 99)
SET(CMAKE_C_COMPILER /usr/bin/mpicc)
add_executable(mkr main.c)
target_link_libraries(mkr m)
target_link_libraries(mkr mpi)

