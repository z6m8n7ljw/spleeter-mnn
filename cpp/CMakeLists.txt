cmake_minimum_required(VERSION 3.10)
project(spleeter-cpp)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

link_directories(lib)

add_executable(spleeter-cpp main.cpp src/Estimator.cpp)

target_link_libraries(spleeter-cpp MNN)
