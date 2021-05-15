cmake_minimum_required(VERSION 3.15)

project(fouracross)

set(Boost_USE_STATIC_LIBS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost 1.71 REQUIRED COMPONENTS system date_time regex)
find_package(Threads REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

add_subdirectory(game)

add_subdirectory(server)
add_subdirectory(client)
