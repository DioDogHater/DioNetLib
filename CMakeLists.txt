cmake_minimum_required(VERSION 3.20)

project(dionetlib)

set(CMAKE_C_STANDARD 11)

add_executable(message-exchange-client samples/message-exchange/client.c)
add_executable(message-exchange-server samples/message-exchange/server.c)

add_executable(multiclient-client samples/multiclient/client.c)
add_executable(multiclient-server samples/multiclient/server.c)

add_executable(multiplayer-game-client samples/multiplayer-game/game.c)
add_executable(multiplayer-game-server samples/multiplayer-game/server.c)
target_link_libraries(multiplayer-game-client SDL2)
target_link_libraries(multiplayer-game-client SDL2_image)
target_link_libraries(multiplayer-game-client SDL2_ttf)