# Overview

This project implements a TCP server and client (using C++ and Boost.Asio) for playing the [Connect Four game](https://en.wikipedia.org/wiki/Connect_Four) over a network.

The server and client use asynchronous socket I/O (through Boost.Asio) but each are polled from a single thread.

## Running the server and client programs

The included solution files create separate executables for the server and client. To play locally, run the server and at least two clients. Gameplay input for the clients is done through the command line.

## Playing the game

When clients connect to the server, they are either put into a game lobby immediately, or into a "matchmaking" queue if all lobbies are full. Once they are put in a game lobby, the client program prompts users to input that they are ready to play. The game starts immediately once both players are ready; the first player is picked randomly.

Games end when there is a winner, the board is full, or a player disconnects mid-game. The players remaining are prompted to stay in the same lobby ("rematch") or quit. 

## Building

Building this project requires Boost - see [Dependencies](#dependencies)

The included Visual Studio solution creates its output in a directory "bin/(Platform)/(Configuration)" in the root directory 
of the solution. 
Build files for other platforms can also be made using the included CMakeLists.

By default, the server is configured to run on the local network using TCP port 8888. If this port can't be used for some reason, "server/testserver.cpp" and "client/testclient.cpp" can be modified to use a different port.

The server is also set to a max of 4 active game lobbies, with 2 players per lobby. The default board size is 5 columns, 4 rows.

## Dependencies

- [Boost 1.71.0](https://www.boost.org/users/history/version_1_71_0.html)
    - *Other versions may work but will also require changing the included Visual Studio project files to point to your version*
    
## Platforms

- The project has been tested on Windows 10 x64 using Visual Studio 2017 (MSVC 14.1).
