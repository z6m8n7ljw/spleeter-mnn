cmake_minimum_required(VERSION 3.10)
project(audio-separation)

set(CMAKE_C_EXTENSIONS      ON)
set(CMAKE_C_STANDARD        11)
set(CMAKE_CXX_EXTENSIONS    ON)
set(CMAKE_CXX_STANDARD      11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

# 配置头文件依赖路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)

# 第三方库依赖路径
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(LINK_LIBRARIES MNN MNN_CL MNN_Express)

add_library(${PROJECT_NAME}
STATIC
""
)

add_subdirectory(src)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
            PRIVATE ${LINK_LIBRARIES})
else()
    target_link_libraries(${PROJECT_NAME}
            PRIVATE ${LINK_LIBRARIES}
            PUBLIC m)
endif()
