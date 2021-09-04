cmake_minimum_required(VERSION 3.17)
project(level_compactor)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)

add_executable(level_loader main.cpp)

add_subdirectory(tinyobjloader)
add_subdirectory(glm)

target_link_libraries(level_loader tinyobjloader)
target_link_libraries(level_loader glm)