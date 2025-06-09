cmake_minimum_required(VERSION 3.5)
project(ffshit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt REQUIRED)
find_package(Iconv REQUIRED)
find_package(spdlog REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/inc
    ${Iconv_INCLUDE_DIRS}
)

set(SRC_LIST 
    src/system.cpp
    src/rawdata.cpp
    src/blocks.cpp 
    src/filesystem/file.cpp
    src/filesystem/directory.cpp
    src/filesystem/sgold.cpp 
    src/filesystem/newsgold.cpp
    src/filesystem/newsgold_x85.cpp
    src/main.cpp)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt spdlog::spdlog ${Iconv_LIBRARIES})
