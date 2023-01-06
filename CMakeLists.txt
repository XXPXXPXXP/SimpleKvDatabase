cmake_minimum_required(VERSION 3.21)
project(Final_project)

set(CMAKE_CXX_STANDARD 23)

add_executable(Final_project master.cpp listener.cpp listener.h serverLog.h serverLog.cpp database.h database.cpp workThread.cpp settings.h master.h Procotol.h Procotol.cpp)
#add_executable(client_test client.cpp client.hpp)

target_link_libraries(Final_project boost)
