cmake_minimum_required(VERSION 3.12)
project(SavingDepth)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(SOURCE_FILES main.cpp)
add_executable(${PROJECT_NAME} main.cpp stb_image_write.h)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries(${PROJECT_NAME} ${DEPENDENCIES} Threads::Threads)
#set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Examples")
#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS ${PROJECT_NAME}  RUNTIME DESTINATION cmake-build-debug ${CMAKE_INSTALL_BINDIR})

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

find_package(realsense2 REQUIRED )
include_directories(include ${realsense_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} ${realsense2_LIBRARY})