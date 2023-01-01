cmake_minimum_required(VERSION 3.21)
project(Final_project)

set(CMAKE_CXX_STANDARD 11)

add_executable(Final_project main.cpp serverCore.cpp serverCore.h main.h serverLog.h serverLog.cpp database.h database.cpp workThread.cpp)
add_executable(client_test client.cpp client.hpp)

target_link_libraries(Final_project libkqueue.so)