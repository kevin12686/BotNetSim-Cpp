cmake_minimum_required(VERSION 3.10)
project(BotNetSim-Cpp)
set(CMAKE_CXX_STANDARD 11)


add_executable(Console Console.cpp)

if (WIN32)
    target_link_libraries(Console ws2_32)
endif ()
add_executable(testing testing.cpp)