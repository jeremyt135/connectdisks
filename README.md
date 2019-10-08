# Overview

connectdisks implements an asynchronous C++ server and client for playing the [Connect Four game](https://en.wikipedia.org/wiki/Connect_Four) over
a network.

## Running the included project

The included solution files and CMakeLists create separate executables for the server and client. The game can be played locally
by running the server first and connecting at least two clients.
Currently, the server only supports 4 active game lobbies, with 2 players per lobby. The default board size is 5 columns, 4 rows. 
(These limits are arbitrary and could be increased if desired.)

## Building

Building this project requires Boost - see [Dependencies](#dependencies)

The included Visual Studio solution creates its output in a directory "build/(Platform)/(Configuration)" in the root directory 
of the solution. 
Build files for other platforms can also be made using the included CMakeLists.

By default, the server is configured to run locally using TCP port 8888. If this port can't be used for some reason, "src/testserver.cpp" 
and "src/testclient.cpp" can be modified to use a different port.

## Dependencies

- [Boost 1.71.0](https://www.boost.org/users/history/version_1_71_0.html)
    - Needed for Boost.Asio, Boost.Endian
    - *Other versions may work but will also require changing the included Visual Studio project files to point to your version*
    
## Platforms

The project has been tested on Windows 10 x64 using Visual Studio 2017 (MSVC 14.1).
