# Overview

This project implements a TCP server and client for playing the [Connect Four game](https://en.wikipedia.org/wiki/Connect_Four).

The server and client use asynchronous socket I/O through Boost.Asio, with each calling `run` on one `io_context` from the main thread.

## Running the server and client programs

The included solution files create separate executables for the server and client. All gameplay is done through the command line.

To run the server: `server PORT`  
To run the client: `client HOST PORT`  

## Playing the game

When clients connect to the server, they are either put into a game lobby immediately, or into a "matchmaking" queue if all lobbies are full. Once they are put in a game lobby, the client program prompts users to input that they are ready to play. The game starts immediately once both players are ready; the first player is picked randomly.

Games end when there is a winner, the board is full, or a player disconnects mid-game. The players remaining are prompted to stay in the same lobby ("rematch") or quit. 

## Building

Included is a CMakeLists.txt file that can be used to generate a Makefile for the project. An out of source build using this file can be done as:  
`cmake . -Bbuild && cd build && make`

## Dependencies

- [Boost 1.71.0](https://www.boost.org/users/history/version_1_71_0.html)
    
## Platforms

- Project has been built and tested on following platforms: 
  - Ubuntu 20.04 LTS (x86_64) - gcc 9.3.0, cmake 3.16.3
