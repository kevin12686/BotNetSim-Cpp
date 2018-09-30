cmake_minimum_required(VERSION 3.10)
project(Bot)
set(CMAKE_CXX_STANDARD 11)

add_library(_thread _thread.h _thread.cpp)
add_library(_socket _socket.h _socket.cpp)
add_library(_socketserver _socketserver.h _socketserver.cpp)


add_executable(Main main.cpp )
add_executable(Multiprocess multiprocessing.cpp )
add_executable(File_Server FileMappingServer.cpp)
add_executable(File_Client FileMappingClient.cpp)
add_executable(Bot  mix_bot.cpp )
if (WIN32)
    target_link_libraries(_socket ws2_32)
    target_link_libraries(Multiprocess _socket ws2_32)
    target_link_libraries(Main _thread _socket _socketserver)
    target_link_libraries(Bot _thread _socket _socketserver)

    set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++")

endif ()