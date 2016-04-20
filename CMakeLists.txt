set(CMAKE_TOOLCHAIN_FILE cmake/toolchain.cmake)

cmake_minimum_required(VERSION 2.8.8)

project(esp-gizmo)

add_subdirectory(mqtt)
add_subdirectory(app)
