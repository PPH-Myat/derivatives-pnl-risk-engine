cmake_minimum_required(VERSION 3.15)
project(derivatives-pnl-risk-engine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    headerFiles
)

# Source files
file(GLOB_RECURSE SOURCES
    sourceFiles/*.cpp
)

# Executable target
add_executable(${PROJECT_NAME} ${SOURCES})