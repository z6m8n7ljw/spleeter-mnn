cmake_minimum_required(VERSION 3.10)
project(spleeter-cpp)

set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

find_package(PkgConfig)
pkg_check_modules(SNDFILE REQUIRED sndfile)

include_directories(include ${SNDFILE_INCLUDE_DIRS})
link_directories(lib ${SNDFILE_LIBRARY_DIRS})

add_executable(spleeter-cpp main.cpp src/Estimator.cpp)

target_link_libraries(spleeter-cpp MNN ${SNDFILE_LIBRARIES})
