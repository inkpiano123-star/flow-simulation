cmake_minimum_required(VERSION 3.5.0...4.0)
project(FlowSim)
set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0135 NEW)
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)
add_executable(FlowSim main.cpp)
target_link_libraries(FlowSim PRIVATE raylib)
add_custom_command(TARGET FlowSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/pipe.obj"
    $<TARGET_FILE_DIR:FlowSim>/pipe.obj)
