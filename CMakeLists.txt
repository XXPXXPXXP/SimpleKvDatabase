cmake_minimum_required(VERSION 3.24)
project(Final_project)

set(CMAKE_CXX_STANDARD 17)

add_executable(Final_project main.cpp server_core.cpp server_core.h main.h server_log.h server_log.cpp database.h database.cpp thread.cpp thread.h)
add_executable(client_test client.cpp client.hpp)

target_link_libraries(Final_project libkqueue.so)