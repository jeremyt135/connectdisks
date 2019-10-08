# required for find_package to work with Boost 1.63 or later
cmake_minimum_required(VERSION 3.7)

project(connectdisks_cmake)

set (Boost_USE_STATIC_LIBS ON)

find_package(Boost REQUIRED system date_time regex)

if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
	add_definitions(-DDEBUG)
endif()

set(CONNECTDISKS_SRC src/board.cpp src/connectdisks.cpp)
set(SERVER_SRC src/server.cpp src/connection.cpp src/gamelobby.cpp ${CONNECTDISKS_SRC})
set(CLIENT_SRC src/client.cpp ${CONNECTDISKS_SRC})

add_executable(connectdisks connectdisks/testconnectdisks.cpp ${CONNECTDISKS_SRC})
target_include_directories(connectdisks PUBLIC "include/")

add_executable(server server/testserver.cpp ${SERVER_SRC})
target_include_directories(server PUBLIC "include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(server ${Boost_LIBRARIES})

add_executable(client client/testclient.cpp ${CLIENT_SRC})
target_include_directories(client PUBLIC "include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(client ${Boost_LIBRARIES})

