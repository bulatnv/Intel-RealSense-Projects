cmake_minimum_required(VERSION 3.12)
project(RGBstreaming)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_executable(${PROJECT_NAME} main.cpp)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

find_package(realsense2 REQUIRED)
include_directories(include ${realsense_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${realsense2_LIBRARY})
