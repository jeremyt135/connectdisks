#include "four-across/networking/server/server.hpp"

#include <boost/asio.hpp>

#include <iostream>

using game::networking::server::Server;

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
		runServer();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	std::cout << "Press enter to exit..." << std::endl;
	std::cin.clear();
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
