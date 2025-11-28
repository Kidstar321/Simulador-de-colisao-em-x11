set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.10)
project(SimColisao C)

set(CMAKE_C_STANDARD 11)

find_package(X11 REQUIRED)

add_executable(SimColisao main.c)

target_link_libraries(SimColisao PRIVATE X11::X11 m)


