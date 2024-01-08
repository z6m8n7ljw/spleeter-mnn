cmake_minimum_required(VERSION 3.10)
project(test-audio-separation)

set(CMAKE_C_EXTENSIONS      ON)
set(CMAKE_C_STANDARD        99)
set(CMAKE_CXX_EXTENSIONS    ON)
set(CMAKE_CXX_STANDARD      11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

if(${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
elseif(${CMAKE_C_COMPILER_ID} MATCHES "^(GNU|.*Clang)$")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror=implicit -std=gnu99 -fPIC")

    if(${CMAKE_C_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_C_COMPILER_VERSION} VERSION_GREATER "6.0.0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wnarrowing -Werror=discarded-qualifiers -Werror=incompatible-pointer-types")
    endif()

    if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_C_COMPILER_ID} STREQUAL "AppleClang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wnarrowing -Werror=ignored-qualifiers -Werror=incompatible-pointer-types")
    endif()

    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fomit-frame-pointer -g")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fomit-frame-pointer -s")

    message(STATUS "gcc/clang cflags ${CMAKE_C_FLAGS}")
    message(STATUS "gcc/clang cflags debug ${CMAKE_C_FLAGS_DEBUG}")
    message(STATUS "gcc/clang cflags release ${CMAKE_C_FLAGS_RELEASE}")
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "^(GNU|.*Clang)$")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=implicit -std=gnu++11 -fno-rtti -fPIC")

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER "6.0.0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnarrowing -Werror=discarded-qualifiers -Werror=incompatible-pointer-types")
  endif()

  if(${CMAKE_CXX_COMPILER_ID} MATCHES ".*Clang$")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnarrowing -Werror=ignored-qualifiers -Werror=incompatible-pointer-types")
  endif()

  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fomit-frame-pointer -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fomit-frame-pointer -s")

  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
  endif()

  message(STATUS "g++/clang++ cppflags ${CMAKE_CXX_FLAGS}")
  message(STATUS "g++/clang++ cppflags debug ${CMAKE_CXX_FLAGS_DEBUG}")
  message(STATUS "g++/clang++ cppflags release ${CMAKE_CXX_FLAGS_RELEASE}")
  message(STATUS "exe linker flags ${CMAKE_EXE_LINKER_FLAGS}")
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

find_package(PkgConfig)
pkg_check_modules(SNDFILE REQUIRED sndfile)

if(NOT LIB_DIR)
    set(LIB_DIR ${CMAKE_SOURCE_DIR}/../)
endif()
if( IS_DIRECTORY ${LIB_DIR} )
    add_subdirectory(${LIB_DIR} lib.out)
else()
    message(FATAL_ERROR   "INVALID FOLDER 'LIB_DIR'=${LIB_DIR}" )
endif()

set(LIB_AUDIO_SEPARATION audio-separation)

include_directories(${LIB_DIR}/include ${SNDFILE_INCLUDE_DIRS})
link_directories(${LIB_DIR}/lib ${SNDFILE_LIBRARY_DIRS})

if(ENABLE_TESTING AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Android" AND NOT IOS)
    add_executable(test-audio-separation test.cpp)
    target_link_libraries(test-audio-separation ${LIB_AUDIO_SEPARATION} ${SNDFILE_LIBRARIES})
endif()

