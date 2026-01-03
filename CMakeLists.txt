cmake_minimum_required(VERSION 3.10)
project(POSHadik C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

include_directories(include)

add_executable(server src/server.c src/game.c src/ipc.c)
add_executable(client src/client.c src/game.c src/ipc.c)

if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
else()
    target_link_libraries(server pthread)
    target_link_libraries(client pthread)
endif()