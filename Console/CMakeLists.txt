cmake_minimum_required(VERSION 3.10)
project(BotNetSim-Cpp)
set(CMAKE_CXX_STANDARD 11)

add_library(Timer Timer.h Timer.cpp)
add_library(_thread _thread.h _thread.cpp)

add_executable(Console Console.cpp)
add_executable(Timer_test Timer_test.cpp)
add_executable(_thread_test _thread_test.cpp)

if (WIN32)
    target_link_libraries(Console ws2_32)
    target_link_libraries(Timer_test Timer)
    target_link_libraries(_thread_test _thread)
endif ()
