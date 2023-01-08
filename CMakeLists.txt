cmake_minimum_required(VERSION 3.21)
project(Final_project)

set(CMAKE_CXX_STANDARD 23)

add_executable(Final_project init.cpp networkIO.cpp networkIO.h serverLog.h serverLog.cpp database.h database.cpp settings.h init.h sender.cpp sender.h reader.cpp reader.h pipeData.h threadpool.h)
