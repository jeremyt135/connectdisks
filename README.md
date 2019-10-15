# Overview

This project implements a TCP server and client (using C++ and Boost.Asio) for playing the [Connect Four game](https://en.wikipedia.org/wiki/Connect_Four) over
a network.

The server and client currently use asynchronous socket I/O (through Boost.Asio) but each are polled from a single thread.

## Running

The included solution files and CMakeLists create separate executables for the server and client. The game can be played locally
by running the server first and connecting at least two clients.
Currently, the server is set to a max of 4 active game lobbies, with 2 players per lobby. The default board size is 5 columns, 4 rows.

## Building

Building this project requires Boost - see [Dependencies](#dependencies)

The included Visual Studio solution creates its output in a directory "build/(Platform)/(Configuration)" in the root directory 
of the solution. 
Build files for other platforms can also be made using the included CMakeLists.

By default, the server is configured to run locally using TCP port 8888. If this port can't be used for some reason, "server/testserver.cpp" 
and "client/testclient.cpp" can be modified to use a different port.

## Dependencies

- [Boost 1.71.0](https://www.boost.org/users/history/version_1_71_0.html)
    - *Other versions may work but will also require changing the included Visual Studio project files to point to your version*
    
## Platforms

- The project has been tested on Windows 10 x64 using Visual Studio 2017 (MSVC 14.1).

## Notes

This is currently a work in progress. It may have bugs producing crashes or other unexpected behavior. 
