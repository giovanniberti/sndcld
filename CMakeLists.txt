cmake_minimum_required(VERSION 3.1)
project(sndcld)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c89 -pedantic -Wall -Wextra")

set(SOURCE_FILES main.c sndcld.c)

find_package(CURL)
include_directories(${CURL_INCLUDE_DIRS})

add_executable(sndcld ${SOURCE_FILES})
target_link_libraries(sndcld ${CURL_LIBRARIES})
