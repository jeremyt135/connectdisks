# Overview

This project implements a TCP server and client for playing the [Connect Four game](https://en.wikipedia.org/wiki/Connect_Four).

The server and client use asynchronous socket I/O through Boost.Asio, with each calling `run` on one `io_context` from the main thread.

# Building

The client & server can be built using the included CMake project or the included Dockerfiles in the respective folders. It's highly recommended to use an out of source build if using CMake.

## Dependencies

- [Boost libraries](https://www.boost.org/users/download/)

# Running the server and client programs

## Locally using built CMake project:

Run the server:<br/>
`build/server/runserver 31001`

Run a client:<br/>
`build/client/runclient 127.0.0.1 31001`

## Using Docker image:

Run the server (assuming image is named `game-svr`):<br/>
`docker run -it --rm --init -p 31001:31001 --name server game-svr`

Run a client (assuming image is named `game-cli`):<br/>
```
SERVER=$(docker inspect -f '{{ .NetworkSettings.IPAddress }}' server)
docker run -it --rm --init game-cli $SERVER 31001
```

# Playing the game

When clients connect to the server, they are either put into a game lobby immediately, or into a queue if all lobbies are full. Once there are 2 players in a lobby, the server requests clients to input that they are ready to play. The game starts immediately once both players are ready; the first player is picked randomly.

Games end when there is a winner, the board is full, or a player disconnects mid-game. The players remaining are prompted to stay in the same lobby ("rematch") or quit.

# Platforms

Currently only Linux is supported. Any recent (1.71+) version of Boost is known to be compatible.
