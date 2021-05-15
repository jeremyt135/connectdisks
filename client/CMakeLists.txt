set(CLIENT_SRC src/client.cpp src/consoleclient.cpp)

add_library(client STATIC ${CLIENT_SRC})
target_include_directories(client PUBLIC "${CMAKE_SOURCE_DIR}/include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(client game ${Boost_LIBRARIES} Threads::Threads)

add_executable(runclient main.cpp)
target_include_directories(runclient PUBLIC "${CMAKE_SOURCE_DIR}/include/" ${Boost_INCLUDE_DIRS})
target_link_libraries(runclient client ${Boost_LIBRARIES})

set_target_properties(client runclient PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)
