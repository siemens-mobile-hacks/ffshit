cmake_minimum_required(VERSION 3.0)
project(ffshit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
)

set(SRC_LIST 
    src/rawdata.cpp
    src/blocks.cpp 
    src/filesystem/file.cpp
    src/filesystem/directory.cpp
    src/filesystem/sgold.cpp 
    src/filesystem/newsgold.cpp
    src/filesystem/newsgold_x85.cpp
    src/main.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})
