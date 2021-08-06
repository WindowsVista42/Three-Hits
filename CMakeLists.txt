cmake_minimum_required(VERSION 3.17)
project(untitled_fps C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-Ofast")
set(CMAKE_C_LINKER_PREFERENCE ld)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_executable(untitled_fps src/main.c src/util.h src/vkutil.h src/vmmath.h)

add_subdirectory(lib/glfw-3.3.4)
find_package(Vulkan REQUIRED)

target_link_libraries(untitled_fps glfw)
target_link_libraries(untitled_fps Vulkan::Vulkan)
