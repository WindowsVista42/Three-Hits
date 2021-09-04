cmake_minimum_required(VERSION 3.17)
project(three_hits C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -DNDEBUG -static -static-libgcc")
set(CMAKE_C_FLAGS_RELEASE "-Ofast -mfpmath=sse -static") #TODO(sean): put the dlls in the folder
#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_executable(
    three_hits
    src/main.c src/util.h src/vkutil.h src/vmmath.h src/state.h src/physics.h src/alutil.h src/init.h
    lib/stb_image.h
        src/update.h src/load.h src/input.h)

add_subdirectory(level_compactor)

add_subdirectory(lib/openal-soft)
add_subdirectory(lib/glfw-3.3.4)
find_package(Vulkan REQUIRED)

target_link_libraries(three_hits OpenAL)
target_link_libraries(three_hits glfw)
target_link_libraries(three_hits Vulkan::Vulkan)
