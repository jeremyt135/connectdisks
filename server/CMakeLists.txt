set(SERVER_SRC src/server.cpp src/connection.cpp src/gamelobby.cpp)

add_library(server STATIC ${SERVER_SRC})
target_include_directories(server PUBLIC "${CMAKE_SOURCE_DIR}/include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(server game ${Boost_LIBRARIES} Threads::Threads)

add_executable(runserver main.cpp)
target_include_directories(runserver PUBLIC "${CMAKE_SOURCE_DIR}/include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(runserver server ${Boost_LIBRARIES})

set_target_properties(server runserver PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
