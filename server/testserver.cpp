#include "connectdisks/server.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace connectdisks;

void runServer()
{
	try
	{
		boost::asio::io_service service;
		Server server{service, "127.0.0.1", 8888};
		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

int main(int argc, char* argv[])
{
	try
	{
		auto server = std::async(std::launch::async, runServer);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}
