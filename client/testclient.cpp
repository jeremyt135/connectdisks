#include "connectdisks/client.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace connectdisks;

void runClient(int id)
{
	try
	{
		std::cout << "starting client " << id << "\n";
		boost::asio::io_service service;
		Client client{service, "127.0.0.1", 8888};
		std::cout << "running client " << id << "\n";
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
		auto client = std::async(std::launch::async, runClient, 1);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
	std::cin.ignore();
}
