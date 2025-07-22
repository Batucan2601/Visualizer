cmake_minimum_required(VERSION 3.10)
project(Visualizer C CXX)  # Enable both C and C++ explicitly

# Use C++17 or later
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard too (optional if raylib needs it)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optional: Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Add raylib (assumes Libraries/raylib is available and has CMakeLists.txt)
add_subdirectory(Libraries/raylib)

# Main executable
add_executable(Visualizer src/main.cpp)

# Link with raylib
target_link_libraries(Visualizer PRIVATE raylib)

# Optional: Include raylib headers
target_include_directories(Visualizer PRIVATE ${CMAKE_SOURCE_DIR}/Libraries/raylib/src)
