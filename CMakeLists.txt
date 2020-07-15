# required for find_package to work with Boost 1.63 or later
cmake_minimum_required(VERSION 3.7)

project(fouracross)

set(Boost_USE_STATIC_LIBS ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost 1.71 REQUIRED COMPONENTS system date_time regex)
find_package(Threads REQUIRED)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DDEBUG)
endif()

set(FOURACROSS_SRC src/board.cpp src/game.cpp)
set(SERVER_SRC src/server.cpp src/connection.cpp src/gamelobby.cpp ${FOURACROSS_SRC})
set(CLIENT_SRC src/client.cpp src/consoleclient.cpp ${FOURACROSS_SRC})

add_executable(server server/run-server.cpp ${SERVER_SRC})
target_include_directories(server PUBLIC "include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(server ${Boost_LIBRARIES} Threads::Threads)

add_executable(client client/run-client.cpp ${CLIENT_SRC})
target_include_directories(client PUBLIC "include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(client ${Boost_LIBRARIES} Threads::Threads)

set_target_properties(server client PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
