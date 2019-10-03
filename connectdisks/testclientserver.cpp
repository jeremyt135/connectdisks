#include "connectdisks/connectdisks.hpp"
#include "connectdisks/client.hpp"
#include "connectdisks/server.hpp"

#include <boost/asio.hpp>

#include <iostream>
#include <thread>

using namespace connectdisks;

std::promise<void> serverIsReady;

void runServer()
{
	try
	{
		boost::asio::io_service service;
		Server server{service, "127.0.0.1", 8888};
		serverIsReady.set_value();
		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}

void runClient()
{
	try
	{
		serverIsReady.get_future().wait();
		boost::asio::io_service service;
		Client client{service, "127.0.0.1", 8888};
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
		auto client = std::async(std::launch::async, runClient);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << "\n";
	}
}