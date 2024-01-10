cmake_minimum_required(VERSION 3.4.1)
project(audio-separation-android)


set(CMAKE_C_EXTENSIONS      ON)
set(CMAKE_C_STANDARD        11)
set(CMAKE_CXX_EXTENSIONS    ON)
set(CMAKE_CXX_STANDARD      11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -fvisibility=hidden")

if(NOT LIB_DIR)
    set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib-audio-separation/)
endif()
if( IS_DIRECTORY ${LIB_DIR} )
    add_subdirectory(${LIB_DIR} lib.out)
else()
    message(FATAL_ERROR   "INVALID FOLDER 'LIB_DIR'=${LIB_DIR}" )
endif()

include_directories(${LIB_DIR}/include)
include_directories(${LIB_DIR}/src)

add_library( # Sets the name of the library.
        ${PROJECT_NAME}
        SHARED
        JniAudioSeparation.cpp)

find_library(log-lib log)
set(AUDIO_SEPARATION audio-separation)

target_link_libraries(${PROJECT_NAME} ${log-lib} ${AUDIO_SEPARATION})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>)