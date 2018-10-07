cmake_minimum_required(VERSION 3.10)
project(Bot)
set(CMAKE_CXX_STANDARD 11)

add_library(_thread _thread.h _thread.cpp)
add_library(_socket _socket.h _socket.cpp SocialNetwork_Bot.cpp)
add_library(_socketserver _socketserver.h _socketserver.cpp SocialNetwork_Bot.cpp)

add_executable(P2PBot P2PBot.cpp )
add_executable(SocialNetwork_Bot  SocialNetwork_Bot.cpp )
add_executable(Multiprocess multiprocessing.cpp )

add_executable(File_Server FileMappingServer.cpp)
add_executable(File_Client FileMappingClient.cpp)

if (WIN32)
    target_link_libraries(_socket ws2_32)
    target_link_libraries(Multiprocess _socket ws2_32)
    target_link_libraries(P2PBot _thread _socket _socketserver)
    target_link_libraries(SocialNetwork_Bot _thread  _socket _socketserver)
    target_link_libraries(File_Client  ws2_32)
    set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++")

endif ()