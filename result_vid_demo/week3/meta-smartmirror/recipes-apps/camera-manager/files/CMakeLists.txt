cmake_minimum_required(VERSION 3.10)
project(camera-manager)

find_package(OpenCV REQUIRED)

add_executable(camera-manager camera-manager.cpp)
target_link_libraries(camera-manager ${OpenCV_LIBS})