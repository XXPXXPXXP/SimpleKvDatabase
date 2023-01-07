cmake_minimum_required(VERSION 3.21)
project(Final_project)

set(CMAKE_CXX_STANDARD 20)

add_executable(Final_project init.cpp listener.cpp listener.h serverLog.h serverLog.cpp database.h database.cpp settings.h init.h sender.cpp sender.h reader.cpp reader.h)

