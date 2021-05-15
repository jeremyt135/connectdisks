set(GAME_SRC src/board.cpp src/game.cpp)

add_library(game STATIC ${GAME_SRC})
target_include_directories(game PUBLIC "${CMAKE_SOURCE_DIR}/include/")

set_target_properties(game PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)