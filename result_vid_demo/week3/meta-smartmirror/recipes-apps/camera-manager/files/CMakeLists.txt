cmake_minimum_required(VERSION 3.10)
project(camera-manager)

set(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{OECORE_TARGET_SYSROOT})
    set(OpenCV_DIR "$ENV{OECORE_TARGET_SYSROOT}/usr/lib/cmake/opencv4")
endif()

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(camera-manager camera-manager.cpp)
target_link_libraries(camera-manager ${OpenCV_LIBS})
